#ifndef HEADER_LOADER_H
#define HEADER_LOADER_H

/* Constants fixed by the PC BIOS. */
#define LOADER_BASE 0x7c00      /* Physical address of loader's base. */
#define LOADER_SIZE 0x200       /* Loader size in bytes (one disk sector). */

/* Physical address of kernel base. */
#define LOADER_KERN_BASE 0x100000       /* 1 MB. */

/* The loader maps 4 MB of the start of physical memory to this
   virtual base address.  Later, the kernel adds the rest of
   physical memory to the mapping.
   This must be aligned on a 4 MB boundary. */
#define LOADER_PHYS_BASE 0xb0000000     /* 3 GB. */

/* Offsets within the loader. */
#define LOADER_BIOS_SIG (LOADER_SIZE - 2)        /* aa55 BIOS signature. */
#define LOADER_CMD_LINE (LOADER_BIOS_SIG - 0x80) /* Kernel command line. */
#define LOADER_RAM_PAGES (LOADER_CMD_LINE - 4)   /* # of pages of RAM. */

#endif /* loader.h */
