#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/mmu.h"
#include "threads/palloc.h"
#include "threads/thread.h"

static thread_func execute_thread NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled before
   process_execute() returns.*/
tid_t
process_execute (const char *filename) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILENAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, filename, PGSIZE);

  /* Create a new thread to execute FILENAME. */
  tid = thread_create (filename, PRI_DEFAULT, execute_thread, fn_copy);
  if (tid == TID_ERROR)
    palloc_free (fn_copy); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
execute_thread (void *filename_)
{
  char *filename = filename_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.es = SEL_UDSEG;
  if_.ds = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  if_.ss = SEL_UDSEG;
  success = load (filename, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  palloc_free (filename);
  if (!success) 
    thread_exit ();

  /* Switch page tables. */
  process_activate ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.pl).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm ("mov %0, %%esp\n"
       "jmp intr_exit\n"
       : /* no outputs */
       : "g" (&if_));
  NOT_REACHED ();
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory.  We have to set
     cur->pagedir to NULL before switching page directories, or a
     timer interrupt might switch back to the process page
     directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_set_esp0 ((uint8_t *) t + PGSIZE);
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
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

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
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

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool load_segment (struct file *, const struct Elf32_Phdr *);
static bool setup_stack (void **esp);

/* Aborts loading an executable, with an error message. */
#define LOAD_ERROR(MSG)                                         \
        do {                                                    \
                printf ("load: %s: ", filename);      \
                printf MSG;                                     \
                printf ("\n");                                  \
                goto done;                                     \
        } while (0)

/* Loads an ELF executable from FILENAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *filename, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL)
    LOAD_ERROR (("page directory allocation failed"));

  /* Open executable file. */
  file = filesys_open (filename);
  if (file == NULL)
    LOAD_ERROR (("open failed"));

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr) 
    LOAD_ERROR (("error reading executable header"));
  if (memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7) != 0)
    LOAD_ERROR (("file is not ELF"));
  if (ehdr.e_type != 2)
    LOAD_ERROR (("ELF file is not an executable"));
  if (ehdr.e_machine != 3)
    LOAD_ERROR (("ELF executable is not x86"));
  if (ehdr.e_version != 1)
    LOAD_ERROR (("ELF executable hasunknown version %d",
                 (int) ehdr.e_version));
  if (ehdr.e_phentsize != sizeof (struct Elf32_Phdr))
    LOAD_ERROR (("bad ELF program header size"));
  if (ehdr.e_phnum > 1024)
    LOAD_ERROR (("too many ELF program headers"));

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      file_seek (file, file_ofs);
      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        LOAD_ERROR (("error reading program header"));
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          /* Reject the executable. */
          LOAD_ERROR (("unsupported ELF segment type %d\n", phdr.p_type));
          break;
        default:
          printf ("unknown ELF segment type %08x\n", phdr.p_type);
          break;
        case PT_LOAD:
          if (!load_segment (file, &phdr))
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage);

/* Loads the segment described by PHDR from FILE into user
   address space.  Return true if successful, false otherwise. */
static bool
load_segment (struct file *file, const struct Elf32_Phdr *phdr) 
{
  void *start, *end;  /* Page-rounded segment start and end. */
  uint8_t *upage;     /* Iterator from start to end. */
  off_t filesz_left;  /* Bytes left of file data (as opposed to
                         zero-initialized bytes). */

  /* Is this a read-only segment?  Not currently used, so it's
     commented out.  You'll want to use it when implementing VM
     to decide whether to page the segment from its executable or
     from swap. */
  //bool read_only = (phdr->p_flags & PF_W) == 0;

  ASSERT (file != NULL);
  ASSERT (phdr != NULL);
  ASSERT (phdr->p_type == PT_LOAD);

  /* [ELF1] 2-2 says that p_offset and p_vaddr must be congruent
     modulo PGSIZE. */
  if (phdr->p_offset % PGSIZE != phdr->p_vaddr % PGSIZE) 
    {
      printf ("%#08"PE32Ox" and %#08"PE32Ax" not congruent modulo %#x\n",
              phdr->p_offset, phdr->p_vaddr, (unsigned) PGSIZE);
      return false; 
    }

  /* [ELF1] 2-3 says that p_memsz must be at least as big as
     p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    {
      printf ("p_memsz (%08"PE32Wx") < p_filesz (%08"PE32Wx")\n",
              phdr->p_memsz, phdr->p_filesz);
      return false; 
    }

  /* Validate virtual memory region to be mapped.
     The region must both start and end within the user address
     space range starting at 0 and ending at PHYS_BASE (typically
     3 GB == 0xc0000000). */
  start = pg_round_down ((void *) phdr->p_vaddr);
  end = pg_round_up ((void *) (phdr->p_vaddr + phdr->p_memsz));
  if (start >= PHYS_BASE || end >= PHYS_BASE || end < start) 
    {
      printf ("bad virtual region %08lx...%08lx\n",
              (unsigned long) start, (unsigned long) end);
      return false; 
    }

  /* Load the segment page-by-page into memory. */
  filesz_left = phdr->p_filesz + (phdr->p_vaddr & PGMASK);
  file_seek (file, ROUND_DOWN (phdr->p_offset, PGSIZE));
  for (upage = start; upage < (uint8_t *) end; upage += PGSIZE) 
    {
      /* We want to read min(PGSIZE, filesz_left) bytes from the
         file into the page and zero the rest. */
      size_t read_bytes = filesz_left >= PGSIZE ? PGSIZE : filesz_left;
      size_t zero_bytes = PGSIZE - read_bytes;
      uint8_t *kpage = palloc_get (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Do the reading and zeroing. */
      if (file_read (file, kpage, read_bytes) != (int) read_bytes) 
        {
          palloc_free (kpage);
          return false; 
        }
      memset (kpage + read_bytes, 0, zero_bytes);
      filesz_left -= read_bytes;

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage)) 
        {
          palloc_free (kpage);
          return false; 
        }
    }

  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free (kpage);
    }
  else
    printf ("failed to allocate process stack\n");

  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.  Fails if UPAGE is
   already mapped or if memory allocation fails. */
static bool
install_page (void *upage, void *kpage)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, true));
}
