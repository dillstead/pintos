#include "addrspace.h"

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification more-or-less literally. */

/* ELF types. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* Executable header.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.
   There are e_phnum of these, starting at file offset e_phoff. */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type in struct Elf32_Phdr. */
#define PT_NULL    0    /* Ignore this program header. */
#define PT_LOAD    1    /* Loadable segment. */
#define PT_DYNAMIC 2    /* Dynamic linking info. */
#define PT_INTERP  3    /* Name of dynamic loader. */
#define PT_NOTE    4    /* Auxiliary info. */
#define PT_SHLIB   5    /* Reserved. */
#define PT_PHDR    6    /* Program header table. */

#define LOAD_ERROR(MSG)                                         \
        do {                                                    \
                printk ("addrspace_load: %s: ", filename);      \
                printk MSG;                                     \
                printk ("\n");                                  \
                goto error;                                     \
        } while (0)

bool
addrspace_load (struct addrspace *as, const char *filename) 
{
  Elf32_Ehdr ehdr;
  struct file *file;

  file = filesys_open (filename);
  if (file == NULL)
    LOAD_ERROR (("open failed"));

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr) 
    LOAD_ERROR (("error reading executable header"));
  if (memcmp (ehdr.e_ident, "\x7fELF\1\1\1", 7) != 0)
    LOAD_ERROR (("not an ELF file"));
  if (ehdr.e_type != 2)
    LOAD_ERROR (("not an executable"));
  if (ehdr.e_machine != 3)
    LOAD_ERROR (("not an x86 binary"));
  if (ehdr.e_version != 1)
    LOAD_ERROR (("unknown ELF version %d", (int) ehdr.e_version));
  if (ehdr.e_phentsize != sizeof (struct Elf32_Phdr))
    LOAD_ERROR (("bad program header size", (int) ehdr.e_phentsize));
  if (ehdr.e_phnum > 1024)
    LOAD_ERROR (("too many program headers"));

  /* Read program headers. */

  as->page_dir = create_page_dir ();
  list_init (&as->vmas);

  
  

}
