#ifndef HEADER_LOADER_H
#define HEADER_LOADER_H

/* Constants fixed by the PC BIOS. */
#define LOADER_BASE 0x7c00      /* Physical address of loader's base. */
#define LOADER_END  0x7e00      /* Physical address of end of loader. */

/* Physical address of kernel base. */
#define LOADER_KERN_BASE 0x100000       /* 1 MB. */

/* Kernel virtual address at which all physical memory is mapped.

   The loader maps the 4 MB at the bottom of physical memory to
   this virtual base address.  Later, paging_init() adds the rest
   of physical memory to the mapping.

   This must be aligned on a 4 MB boundary. */
#define LOADER_PHYS_BASE 0xc0000000     /* 3 GB. */

/* Offsets within the loader. */
#define LOADER_BIOS_SIG (LOADER_END - 2)        /* 0xaa55 BIOS signature. */
#define LOADER_CMD_LINE_LEN 0x80                /* Command line length. */
#define LOADER_CMD_LINE (LOADER_BIOS_SIG - LOADER_CMD_LINE_LEN)
                                                /* Kernel command line. */
#define LOADER_RAM_PAGES (LOADER_CMD_LINE - 4)  /* # of pages of RAM. */

#endif /* loader.h */
