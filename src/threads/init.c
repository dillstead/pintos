#include "init.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include "debug.h"
#include "interrupt.h"
#include "io.h"
#include "kbd.h"
#include "lib.h"
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
#endif

/* Size of kernel static code and data, in 4 kB pages. */
size_t kernel_pages;

/* Amount of physical memory, in 4 kB pages. */
size_t ram_pages;

static void gdt_init (void);
void power_off (void);

static void
main_thread (void *aux UNUSED) 
{
  printk ("execute=%d\n", (int) thread_execute ("a.out"));
}

int
main (void)
{
  extern char _text, _end, __bss_start;
  struct thread *t;

  /* Clear out the BSS segment. */
  memset (&__bss_start, 0, &_end - &__bss_start);

  vga_init ();
  serial_init ();

  /* Calculate how much RAM the kernel uses, and find out from
     the bootloader how much RAM this machine has. */
  kernel_pages = (&_end - &_text + 4095) / 4096;
  ram_pages = *(uint32_t *) (0x7e00 - 8);

  printk ("Initializing nachos-x86, %d kB RAM detected.\n",
          ram_pages * 4);

  /* Memory from the end of the kernel through the end of memory
     is free.  Give it to the page allocator. */
  palloc_init ((void *) (KERN_BASE + kernel_pages * PGSIZE),
               (void *) (PHYS_BASE + ram_pages * PGSIZE));
  paging_init ();
  gdt_init ();

  malloc_init ();
  random_init ();

  intr_init ();
  timer_init ();
  kbd_init ();

#ifdef FILESYS
  filesys_init (false);
  filesys_self_test ();
#endif

  thread_init ();

  t = thread_create ("main", main_thread, NULL);
  thread_start (t);

  printk ("Done!\n");
  return 0;
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
