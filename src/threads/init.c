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

static void init_page_table (void);
static void setup_gdt (void);
void power_off (void);

struct thread *a, *b;

static void
tfunc (void *aux UNUSED) 
{
  for (;;) 
    {
      size_t count, i;
      if (random_ulong () % 5 == 0)
        {
          printk ("%s exiting\n", thread_current ()->name);
          break;
        }
      count = random_ulong () % 25 * 10000;
      printk ("%s waiting %zu: ", thread_current ()->name, count);
      for (i = 0; i < count; i++);
      printk ("%s\n", thread_current ()->name);
    }
}

int
main (void)
{
  extern char _text, _end, __bss_start;

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
  palloc_init ((void *) (KERN_BASE + kernel_pages * NBPG),
               (void *) (PHYS_BASE + ram_pages * NBPG));

  init_page_table ();
  setup_gdt ();

  malloc_init ();
  random_init ();

  intr_init ();
  timer_init ();
  kbd_init ();

#ifdef FILESYS
  filesys_init (false);
#endif

  thread_init ();

  {
    struct thread *t;
    int i;
    
    for (i = 0; i < 4; i++) 
      {
        char name[2];
        name[0] = 'a' + i;
        name[1] = 0;
        t = thread_create (name, tfunc, NULL); 
      }
    thread_start (t); 
  }

  printk ("Done!\n");
  return 0;
}

/* Populates the page directory and page table with the kernel
   virtual mapping. */
static void
init_page_table (void)
{
  uint32_t *pd, *pt;
  uint32_t paddr;

  pd = palloc_get (PAL_ASSERT | PAL_ZERO);
  pt = NULL;
  for (paddr = 0; paddr < NBPG * ram_pages; paddr += NBPG)
    {
      uint32_t vaddr = paddr + PHYS_BASE;
      size_t pde_idx = PDENO(vaddr);
      size_t pte_idx = PTENO(vaddr);

      if (pd[pde_idx] == 0)
        {
          pt = palloc_get (PAL_ASSERT | PAL_ZERO);
          pd[pde_idx] = (uint32_t) vtop (pt) | PG_U | PG_W | PG_P;
        }

      pt[pte_idx] = paddr | PG_U | PG_W | PG_P;
    }

  /* Set the page table. */
  asm volatile ("movl %0,%%cr3" :: "r" (vtop (pd)));
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
                 | ( system << 12)  /* 0=system, 1=code/data. */
                 | ( type << 8)     /* Segment type. */
                 | (dpl << 13)                /* Descriptor privilege. */
                 | (1 << 15)                  /* Present. */
                 | (limit & 0xf0000)          /* Limit 16:19. */
                 | (1 << 22)                  /* 32-bit segment. */
                 | ( granularity << 23) /* Byte/page granularity. */
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
make_tss_desc (uint32_t base)
{
  return make_seg_desc (base, 0x67, SYS_SYSTEM, TYPE_TSS_32_A, 0, GRAN_BYTE);
}

uint64_t gdt[SEL_CNT];

struct tss *tss;

/* Sets up a proper GDT.  The bootstrap loader's GDT didn't
   include user-mode selectors or a TSS. */
static void
setup_gdt (void)
{
  uint64_t gdtr_operand;

  /* Our TSS is never used in a call gate or task gate, so only a
     few fields of it are ever referenced, and those are the only
     ones we initialize. */
  tss = palloc_get (PAL_ASSERT | PAL_ZERO);
  tss->esp0 = (uint32_t) ptov(0xc0020000);
  tss->ss0 = SEL_KDSEG;
  tss->bitmap = 0xdfff;

  /* Initialize GDT. */
  gdt[SEL_NULL / sizeof *gdt] = 0;
  gdt[SEL_KCSEG / sizeof *gdt] = make_code_desc (0);
  gdt[SEL_KDSEG / sizeof *gdt] = make_data_desc (0);
  gdt[SEL_UCSEG / sizeof *gdt] = make_code_desc (3);
  gdt[SEL_UDSEG / sizeof *gdt] = make_data_desc (3);
  gdt[SEL_TSS / sizeof *gdt] = make_tss_desc (vtop (tss));

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
