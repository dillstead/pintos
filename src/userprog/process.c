#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/pageinfo.h"
#include "vm/mmap.h"

/* Maximum size of program arguments. */
#define MAX_ARGS_SIZE 512

struct start_args
{
  char *program_name;
  char *program_args;
  tid_t ptid;
  struct semaphore start_wait;
  struct thread *child;
};

static thread_func start_process NO_RETURN;
static bool load (char *program_name, char *program_args, void (**eip) (void),
                  void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name_) 
{
  struct thread *cur = thread_current ();
  char *file_name;
  struct start_args args;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  file_name = palloc_get_page (0);
  if (file_name == NULL)
    return TID_ERROR;
  strlcpy (file_name, file_name_, PGSIZE);
  args.program_name = strtok_r (file_name, " ", &args.program_args);
  args.ptid = cur->tid;

  /* Create a new thread to execute FILE_NAME. */
  sema_init (&args.start_wait, 0);
  tid = thread_create (args.program_name, PRI_DEFAULT, start_process, &args);
  if (tid != TID_ERROR)
    {
      /* Wait for the thread to start running so it can be added to the child
         list. */
      sema_down (&args.start_wait);
      if (args.child != NULL)  
        list_push_back (&cur->child_list, &args.child->child_elem);
      else
        tid = TID_ERROR;
    }
  palloc_free_page (file_name); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *args_)
{
  struct thread *cur = thread_current ();
  struct start_args *args = args_;
  struct intr_frame if_;
  bool success = false;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  if (load (args->program_name, args->program_args, &if_.eip, &if_.esp))
    {
      args->child = cur;
      cur->ptid = args->ptid;
      success = true;
    }
  else
    args->child = NULL;
  sema_up (&args->start_wait);

  /* If load failed, quit. */
  if (!success)
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{
  struct thread *cur = thread_current ();
  struct thread *child;
  struct list_elem *e;
  int exit_status;

  for (child = NULL, e = list_begin (&cur->child_list);
       e != list_end (&cur->child_list); e = list_next (e))
    {
      child = list_entry (e, struct thread, child_elem);
      if (child->tid == child_tid)
        {
          list_remove (e);
          break;
        }
    }
  if (child != NULL)
    {
      /* If the child has not yet exited, wait for it to
         exit. */
      lock_acquire (&child->exit_lock);
      while (child->status != THREAD_EXITING)
        cond_wait (&child->exiting, &child->exit_lock);
      lock_release (&child->exit_lock);
      /* At this point it's safe to free the child since
         it's no longer running and will never be rescheduled. */
      exit_status = child->exit_status;
      palloc_free_page (child);
    }
  else
    exit_status = -1; /* Child not found. */

  return exit_status;
}

/* Free the current process's resources.  This function is called by
   and works in conjunction with thread_exit.  As such, it relies 
   on thread_exit() to free the process' stack if applicable. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;
  int fd;
  int md;
  struct list_elem *e;
  enum thread_status status;

  if (cur->mfiles != NULL)
    {
      for (md = 0; md < MAX_MMAP_FILES; md++)
        munmap (md);
      free (cur->mfiles);
    }
  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
      printf ("%s: exit(%d)\n", cur->name, cur->exit_status);
    }
  if (cur->ofiles != NULL)
    {
      for (fd = 2; fd < MAX_OPEN_FILES; fd++)
        process_file_close (fd);
      free (cur->ofiles);
    }

  lock_acquire (&cur->exit_lock);
  for (e = list_begin (&cur->child_list); e != list_end (&cur->child_list);
       e = list_remove (e))
    {
      struct thread *child = list_entry (e, struct thread, child_elem);
      
      lock_acquire (&child->exit_lock);
      /* Orphan the child so when it exits it won't bother signaling the 
         parent. */
      child->ptid = TID_NONE;
      if (child->status == THREAD_EXITING)
        /* It's safe to free an exited child's stack. */
        palloc_free_page (child);
      lock_release (&child->exit_lock);
    }
  
  if (cur->ptid != TID_NONE)
    {
      /* The parent has not yet exited, signal it if it's waiting
         on this process.  As the stack needs to be valid for the 
         parent to read exit_status, it's the parent's responsibility to
         free the stack now.  Marking the status as THREAD_EXITING
         ensures that it won't be freed in thread_exit(). */
      status = THREAD_EXITING;
      cond_signal (&cur->exiting, &cur->exit_lock);
    }
  else
    /* Marking the status as THREAD_DYING ensures that it will be freed
       in thread_exit(). */
    status = THREAD_DYING;
  
  /* Interrupts must be disabled before releasing the lock.  If they
     aren't it's possible for the parent waiting on us to free our 
     stack before we've been scheduled away in thread_exit(). */
  intr_disable();
  lock_release (&cur->exit_lock);
  cur->status = status;
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *cur = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (cur->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}


/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Enlf32_Addr in hexadecimal. */
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

static bool setup_stack (const char *program_name, char *args, void **esp);
static bool validate_segment (const struct Elf32_Phdr *, int fd);
static bool load_segment (int fd, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (char *program_name, char *program_args, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  off_t file_ofs;
  bool success = false;
  int i;
  int fd;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  t->ofiles = calloc (MAX_OPEN_FILES, sizeof *t->ofiles);
  if (t->ofiles == NULL)
    goto done;
  t->mfiles = calloc (MAX_MMAP_FILES, sizeof *t->mfiles);
  if (t->mfiles == NULL)
    goto done;
  /* Open executable file as read-only so it can't be modified
     while the process is running. */
  fd = process_file_open (program_name, true);
  if (fd == -1)
    {
      printf ("load: %s: open failed\n", program_name);
      goto done;       
    }

  /* Read and verify executable header. */
  if (process_file_read (fd, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", program_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > process_file_size (fd))
        goto done;
      process_file_seek (fd, file_ofs);

      if (process_file_read (fd, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, fd)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (fd, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }
  
  /* Set up stack. */
  if (!setup_stack (program_name, program_args, esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, int fd) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) process_file_size (fd)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

   - READ_BYTES bytes at UPAGE must be read from FILE
   starting at offset OFS.

   - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error occurs.
*/
static bool
load_segment (int fd, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  struct thread *cur = thread_current ();
  struct page_info *page_info;
  struct file *file;
    
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file = process_file_get_file (fd);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      page_info = pageinfo_create ();
      if (page_info == NULL)
        return false;
      
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      pageinfo_set_pagedir (page_info, cur->pagedir);
      pageinfo_set_upage (page_info, upage);
      if (page_read_bytes > 0)
        {
          ofs += page_read_bytes;
          pageinfo_set_type (page_info, PAGE_TYPE_FILE);
          pageinfo_set_fileinfo (page_info, file, ofs);
        }
      else
        pageinfo_set_type (page_info, PAGE_TYPE_ZERO);
      if (writable)
        pageinfo_set_writable (page_info, WRITABLE_TO_SWAP);
      pagedir_set_info (cur->pagedir, upage, page_info);
      
      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory and adding the program name and arguments. */
static bool
setup_stack (const char *program_name, char *args, void **esp)
{
  struct thread *cur = thread_current ();
  struct page_info *page_info;
  const char *arg;
  uint8_t *kpage, *top, *bottom, *base;
  void *upage;
  size_t len;
  int argc;
  bool success = false;

  kpage = palloc_get_page (PAL_ZERO);
  page_info = pageinfo_create ();
  if (kpage != NULL && page_info != NULL)
    {
      upage = (void *) (PHYS_BASE - PGSIZE);
      success = install_page (upage, kpage, true);
      if (success)
        {
          bottom = PHYS_BASE;
          top = PHYS_BASE - MAX_ARGS_SIZE;
          base = top;
          argc = 0;
          arg = program_name;
          while (arg != NULL)
            {
              len = strlen (arg) + 1;
              /* In addition to the pointer for this arg, make sure to account
                 for the final NULL arg pointer when increasing top. */
              if (word_round_down (bottom - len)
                  <= (void *) (top + 2 * sizeof (char *)))
                /* No more space left for argument, drop it and finish. */
                break;
              bottom -= len;
              memcpy (bottom, arg, len * sizeof (char));
              *((uint8_t **) top) = bottom;
              top += sizeof (char *);
              argc++;
              arg = args != NULL ? strtok_r (NULL, " ", &args) : NULL;
            }
          bottom = word_round_down (bottom) - sizeof (char *);
          /* Move the arg pointers to their proper spot, word aligned below
             the final NULL arg pointer. */
          memmove (base + (bottom - top), base, argc * sizeof (char *));
          base += bottom - top;
          /* Fill ret, argc, and argv. */
          *((uint8_t **) (base - sizeof (char**))) = base;
          base -= sizeof (char **) + sizeof (argc);
          *((int *) base) = argc;
          base -= sizeof (void *);
          *base = 0;
          *esp = base;

          /* Setup the page info and allow the stack to be properly paged
             in during a page fault. */
          pageinfo_set_upage (page_info, upage);
          pageinfo_set_pagedir (page_info, cur->pagedir);
          pageinfo_set_type (page_info, PAGE_TYPE_KERNEL);
          pageinfo_set_kpage (page_info, kpage);
          pageinfo_set_writable (page_info, WRITABLE_TO_SWAP);
          pagedir_set_info (cur->pagedir, upage, page_info);
          pagedir_clear_page (cur->pagedir, upage);
        }
      else
        {
          pageinfo_destroy (page_info);
          palloc_free_page (kpage);
        }
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
