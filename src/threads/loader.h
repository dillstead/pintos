#ifndef THREADS_LOADER_H
#define THREADS_LOADER_H

/* Physical addresses of important kernel components. */
#define LOADER_PD_BASE   0x0f000 /* Page directory: 4 kB starting at 60 kB. */
#define LOADER_PT_BASE   0x10000 /* Page tables: 64 kB starting at 64 kB. */
#define LOADER_KERN_BASE 0x20000 /* Kernel: up to 512 kB starting at 128 kB. */

/* Kernel virtual address at which all physical memory is mapped.

   The loader maps the 4 MB at the bottom of physical memory to
   this virtual base address.  Later, paging_init() adds the rest
   of physical memory to the mapping.

   This must be aligned on a 4 MB boundary. */
#define LOADER_PHYS_BASE 0xc0000000     /* 3 GB. */

/* GDT selectors defined by loader.
   More selectors are defined by userprog/gdt.h. */
#define SEL_NULL        0x00    /* Null selector. */
#define SEL_KCSEG       0x08    /* Kernel code selector. */
#define SEL_KDSEG       0x10    /* Kernel data selector. */

#endif /* threads/loader.h */
