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
#include "fsutil.h"
#endif

/* Amount of physical memory, in 4 kB pages. */
size_t ram_pages;

#ifdef FILESYS
/* Format the filesystem? */
static bool format_filesys;
#endif

#ifdef USERPROG
/* Initial program to run. */
static char *initial_program;
#endif

static void ram_init (void);
static void gdt_init (void);
static void argv_init (void);

static void
main_thread (void *aux UNUSED) 
{
#ifdef FILESYS
  disk_init ();
  filesys_init (format_filesys);
  fsutil_run ();
#endif

#ifdef USERPROG
  if (initial_program != NULL)
    thread_execute (initial_program);
  else
    PANIC ("no initial program specified");
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

  /* Parse command line. */
  argv_init ();

  /* Initialize memory system. */
  palloc_init ();
  paging_init ();
  gdt_init ();
  malloc_init ();

  random_init (0);

  /* Initialize interrupt handlers. */
  intr_init ();
  timer_init ();
  kbd_init ();

  /* Do everything else in a system thread. */
  thread_init ();
  thread_create ("main", main_thread, NULL);
  thread_start ();
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

static uint64_t gdt[SEL_CNT];

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

static void
argv_init (void) 
{
  char *cmd_line, *pos;
  char *argv[LOADER_CMD_LINE_LEN / 2 + 1];
  int argc = 0;
  int i;

  /* The command line is made up of null terminated strings
     followed by an empty string.  Break it up into words. */
  cmd_line = pos = ptov (LOADER_CMD_LINE);
  while (pos < cmd_line + LOADER_CMD_LINE_LEN)
    {
      ASSERT (argc < LOADER_CMD_LINE_LEN / 2);
      if (*pos == '\0')
        break;
      argv[argc++] = pos;
      pos = strchr (pos, '\0') + 1;
    }
  argv[argc] = "";

  /* Parse the words. */
  for (i = 0; i < argc; i++)
    if (!strcmp (argv[i], "-rs")) 
      random_init (atoi (argv[++i]));
    else if (!strcmp (argv[i], "-d")) 
      debug_enable (argv[++i]);
#ifdef USERPROG
    else if (!strcmp (argv[i], "-ex")) 
      initial_program = argv[++i];
#endif
#ifdef FILESYS
  else if (!strcmp (argv[i], "-f"))
      format_filesys = true;
    else if (!strcmp (argv[i], "-cp")) 
      fsutil_copy_arg = argv[++i];
    else if (!strcmp (argv[i], "-p")) 
      fsutil_print_file = argv[++i];
    else if (!strcmp (argv[i], "-r"))
      fsutil_remove_file = argv[++i];
    else if (!strcmp (argv[i], "-ls"))
      fsutil_list_files = true;
    else if (!strcmp (argv[i], "-D"))
      fsutil_dump_filesys = true;
#endif
    else if (!strcmp (argv[i], "-u"))
      {
        printk (
          "Kernel options:\n"
          " -rs SEED            Seed random seed to SEED.\n"
          " -d CLASS[,...]      Enable the given classes of debug messages.\n"
#ifdef USERPROG
          " -ex 'PROG [ARG...]' Run PROG, passing the optional arguments.\n"
#endif
#ifdef FILESYS
          " -f                  Format the filesystem disk (hdb or hd0:1).\n"
          " -cp FILENAME:SIZE   Copy SIZE bytes from the scratch disk (hdc\n"
          "                     or hd1:0) into the filesystem as FILENAME\n"
          " -p FILENAME         Print the contents of FILENAME\n"
          " -r FILENAME         Delete FILENAME\n"
          " -ls                 List the files in the filesystem\n"
          " -D                  Dump complete filesystem contents\n");
#endif
      }
    else 
      PANIC ("unknown option `%s'", argv[i]);
}
