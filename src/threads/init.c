#include "init.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include "debug.h"
#include "interrupt.h"
#include "io.h"
#include "kbd.h"
#include "lib.h"
#include "loader.h"
#include "malloc.h"
#include "mmu.h"
#include "paging.h"
#include "palloc.h"
#include "random.h"
#include "serial.h"
#include "thread.h"
#include "timer.h"
#include "vga.h"
#ifdef FILESYS
#include "filesys.h"
#include "disk.h"
#endif

/* Amount of physical memory, in 4 kB pages. */
size_t ram_pages;

static void ram_init (void);
static void gdt_init (void);
static void argv_init (void);
void power_off (void);

static void
main_thread (void *aux UNUSED) 
{
#ifdef FILESYS
  disk_init ();
  filesys_init (true);
  filesys_self_test ();
#endif
}

int
main (void)
{
  /* Initialize prerequisites for calling printk(). */
  ram_init ();
  vga_init ();
  serial_init ();

  /* Greet user. */
  printk ("Booting cnachos86 with %'d kB RAM...\n", ram_pages * 4);

  /* Initialize memory system. */
  palloc_init ();
  paging_init ();
  gdt_init ();
  malloc_init ();

  random_init ();
  argv_init ();

  /* Initialize interrupt handlers. */
  intr_init ();
  timer_init ();
  kbd_init ();

  /* Do everything else in a system thread. */
  thread_init ();
  thread_start (thread_create ("main", main_thread, NULL));
}

static uint64_t
make_seg_desc (uint32_t base,
               uint32_t limit,
               enum seg_system system,
               enum seg_type type,
               int dpl,
               enum seg_granularity granularity)
{
  uint32_t e0 = ((limit & 0xffff)             /* Limit 15:0. */
                 | (base << 16));             /* Base 15:0. */
  uint32_t e1 = (((base >> 16) & 0xff)        /* Base 23:16. */
                 | (system << 12)             /* 0=system, 1=code/data. */
                 | (type << 8)                /* Segment type. */
                 | (dpl << 13)                /* Descriptor privilege. */
                 | (1 << 15)                  /* Present. */
                 | (limit & 0xf0000)          /* Limit 16:19. */
                 | (1 << 22)                  /* 32-bit segment. */
                 | (granularity << 23)        /* Byte/page granularity. */
                 | (base & 0xff000000));      /* Base 31:24. */
  return e0 | ((uint64_t) e1 << 32);
}

static uint64_t
make_code_desc (int dpl)
{
  return make_seg_desc (0, 0xfffff, SYS_CODE_DATA, TYPE_CODE | TYPE_READABLE,
                        dpl, GRAN_PAGE);
}

static uint64_t
make_data_desc (int dpl)
{
  return make_seg_desc (0, 0xfffff, SYS_CODE_DATA, TYPE_WRITABLE,
                        dpl, GRAN_PAGE);
}

static uint64_t
make_tss_desc (void *vaddr)
{
  return make_seg_desc ((uint32_t) vaddr,
                        0x67, SYS_SYSTEM, TYPE_TSS_32_A, 0, GRAN_BYTE);
}

uint64_t gdt[SEL_CNT];

struct tss *tss;

/* Sets up a proper GDT.  The bootstrap loader's GDT didn't
   include user-mode selectors or a TSS. */
static void
gdt_init (void)
{
  uint64_t gdtr_operand;

  /* Our TSS is never used in a call gate or task gate, so only a
     few fields of it are ever referenced, and those are the only
     ones we initialize. */
  tss = palloc_get (PAL_ASSERT | PAL_ZERO);
  tss->esp0 = (uint32_t) ptov(0x20000);
  tss->ss0 = SEL_KDSEG;
  tss->bitmap = 0xdfff;

  /* Initialize GDT. */
  gdt[SEL_NULL / sizeof *gdt] = 0;
  gdt[SEL_KCSEG / sizeof *gdt] = make_code_desc (0);
  gdt[SEL_KDSEG / sizeof *gdt] = make_data_desc (0);
  gdt[SEL_UCSEG / sizeof *gdt] = make_code_desc (3);
  gdt[SEL_UDSEG / sizeof *gdt] = make_data_desc (3);
  gdt[SEL_TSS / sizeof *gdt] = make_tss_desc (tss);

  /* Load GDTR, TR. */
  gdtr_operand = make_dtr_operand (sizeof gdt - 1, gdt);
  asm volatile ("lgdt %0" :: "m" (gdtr_operand));
  asm volatile ("ltr %w0" :: "r" (SEL_TSS));
}

static void
ram_init (void) 
{
  /* The "BSS" is a segment that should be initialized to zeros.
     It isn't actually stored on disk or zeroed by the kernel
     loader, so we have to zero it ourselves.

     The start and end of the BSS segment is recorded by the
     linker as _start_bss and _end_bss.  See kernel.lds. */
  extern char _start_bss, _end_bss;
  memset (&_start_bss, 0, &_end_bss - &_start_bss);

  /* Get RAM size from loader. */
  ram_pages = *(uint32_t *) ptov (LOADER_RAM_PAGES);
}

/* This should be sufficient because the command line buffer is
   only 128 bytes and arguments are space-delimited. */
#define ARGC_MAX 64

int argc;
char *argv[ARGC_MAX + 1];

static void
argv_init (void) 
{
  char *cmd_line = ptov (LOADER_CMD_LINE);
  char *arg, *pos;

  for (arg = strtok_r (cmd_line, " \t\r\n\v", &pos); arg != NULL;
       arg = strtok_r (NULL, " \t\r\n\v", &pos))
    {
      ASSERT (argc < ARGC_MAX);
      argv[argc++] = arg;
    }
  argv[argc] = NULL;
}

void
power_off (void) 
{
  const char s[] = "Shutdown";
  const char *p;

  printk ("Powering off...\n");
  for (p = s; *p != '\0'; p++)
    outb (0x8900, *p);
  for (;;);
}
