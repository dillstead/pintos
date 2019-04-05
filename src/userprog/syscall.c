#include "userprog/syscall.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

/* Serialize all file system operations since they are 
not thread safe. */
static struct lock filesys_lock;

static void syscall_handler (struct intr_frame *);

static int sys_halt(const uint8_t *arg_base);
static int sys_exit(const uint8_t *arg_base);
static int sys_exec(const uint8_t *arg_base);
static int sys_wait(const uint8_t *arg_base);
static int sys_create(const uint8_t *arg_base);
static int sys_remove(const uint8_t *arg_base);
static int sys_open(const uint8_t *arg_base);
static int sys_filesize(const uint8_t *arg_base);
static int sys_read(const uint8_t *arg_base);
static int sys_write(const uint8_t *arg_base);
static int sys_seek(const uint8_t *arg_base);
static int sys_tell(const uint8_t *arg_base);
static int sys_close(const uint8_t *arg_base);

static int (*syscalls[])(const uint8_t *arg_base) =
{
  [SYS_HALT] sys_halt,
  [SYS_EXIT] sys_exit,
  [SYS_EXEC] sys_exec,
  [SYS_WAIT] sys_wait,
  [SYS_CREATE] sys_create,
  [SYS_REMOVE] sys_remove,
  [SYS_OPEN] sys_open, 
  [SYS_FILESIZE] sys_filesize,
  [SYS_READ] sys_read,
  [SYS_WRITE] sys_write,
  [SYS_SEEK] sys_seek,
  [SYS_TELL] sys_tell,
  [SYS_CLOSE] sys_close
};

void
syscall_init (void) 
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

static bool
read_int (const uint8_t *uaddr, int *pi)
{
  int b0, b1, b2, b3;

  /* Outside valid range. */
  if (!is_user_vaddr (uaddr) || !is_user_vaddr (uaddr + sizeof (int)))
    return false;
  
  b0 = get_user (uaddr);
  b1 = get_user (uaddr + 1);
  b2 = get_user (uaddr + 2);
  b3 = get_user (uaddr + 3);

  /* Invalid address. */
  if (b0 == -1 || b1 == -1 || b2 == -1 || b3 == -1)
    return false;

  *pi = (uint8_t) b0 | (uint8_t) b1 << 8 | (uint8_t) b2 << 16
    | (uint8_t) b3 << 24;
  return true;
}

static bool
read_byte (const uint8_t *uaddr, uint8_t *pb)
{
  int b;

  /* Outside valid range. */
  if (!is_user_vaddr (uaddr))
    return false;
  
  b = get_user (uaddr);

  /* Invalid address. */
  if (b == -1)
    return false;

  *pb = (uint8_t) b;
  return true;
}

static bool
read_char (const uint8_t *uaddr, char *pc)
{
  return read_byte (uaddr, (uint8_t *) pc);
}

static bool
write_byte (uint8_t *uaddr, uint8_t b)
{
  return is_user_vaddr (uaddr) && put_user (uaddr, b);
}

static bool
get_int_arg (const uint8_t *uaddr, int pos, int *pi)
{
  return read_int (uaddr + sizeof (int) * pos, pi);
}

static bool
get_str_arg (const uint8_t *uaddr, int pos, char **pstr)
{
  uint8_t *ustraddr;
  char c;
  
  if (!get_int_arg (uaddr, pos, (int *) pstr))
    return false;

  ustraddr = (uint8_t *) *pstr;

  while (read_char (ustraddr++, &c) && c != '\0');
  return c == '\0' ? true : false;
}

static bool
get_ptr_arg (const uint8_t *uaddr, int pos, uint8_t **pp)
{
  return get_int_arg (uaddr, pos, (int *) pp);
}

static bool
get_read_buf_arg (const uint8_t *uaddr, int pos, uint8_t **pp, unsigned size)
{
  uint8_t *puaddr;
  uint8_t b;
  
  if (!get_ptr_arg (uaddr, pos, pp))
    return false;

  puaddr = *pp;

  return read_byte (puaddr, &b) && read_byte (puaddr + size, &b);
}

static bool
get_write_buf_arg (const uint8_t *uaddr, int pos, uint8_t **pp, unsigned size)
{
  uint8_t *puaddr;
  
  if (!get_ptr_arg (uaddr, pos, pp))
    return false;

  puaddr = *pp;

  return write_byte (puaddr, 0) && write_byte (puaddr + size, 0);
}

static void
syscall_handler (struct intr_frame *f) 
{
  unsigned num;

  if (!get_int_arg (f->esp, 0, (int *) &num))
    {
      thread_exit ();
    }
  if (num > 0 && num < sizeof syscalls / sizeof *syscalls
      && syscalls[num] != NULL)
    f->eax = syscalls[num] ((uint8_t *) f->esp + sizeof (int));
  else
    f->eax = -1;
}

static
int sys_halt(const uint8_t *arg_base)
{
  (void) arg_base;

  shutdown_power_off ();
  NOT_REACHED ();
  
  return 0;
}

static
int sys_exit(const uint8_t *arg_base)
{
  int status;

  if (!get_int_arg (arg_base, 0, &status))
    thread_exit ();
  
  thread_current ()->exit_status = status;
  thread_exit ();
  NOT_REACHED ();

  return 0;
}

static
int sys_exec(const uint8_t *arg_base)
{
  char *cmd_line;
  
  if (!get_str_arg (arg_base, 0, &cmd_line))
    thread_exit ();

  return process_execute (cmd_line);
}

static
int sys_wait(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}

static
int sys_create(const uint8_t *arg_base)
{
  char *name;
  off_t initial_size;
  bool success;

  if (!get_str_arg (arg_base, 0, &name)
      || !get_int_arg (arg_base, 1, (int *) &initial_size))
    thread_exit ();
  
  lock_acquire (&filesys_lock);
  success = filesys_create (name, initial_size);
  lock_release (&filesys_lock);
  return success;
}

static
int sys_remove(const uint8_t *arg_base)
{
  char *name;
  bool success;

  if (!get_str_arg (arg_base, 0, &name))
    thread_exit ();

  lock_acquire (&filesys_lock);
  success = filesys_create (name, initial_size);
  lock_release (&filesys_lock);
  return success;
}

static
int sys_open(const uint8_t *arg_base)
{
  char *name;

  if (!get_str_arg (arg_base, 0, &name))
    thread_exit ();

  lock_acquire (&filesys_lock);
  success = filesys_create (name, initial_size);
  lock_release (&filesys_lock);
  return success;
  
}

static
int sys_filesize(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}

static
int sys_read(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}

static
int sys_write(const uint8_t *arg_base)
{
  int fd, written;
  char *buffer;
  unsigned size;
  
  if (!get_int_arg (arg_base, 0, &fd)
      || !get_ptr_arg (arg_base, 1, (uint8_t ** )&buffer)
      || !get_int_arg (arg_base, 2, (int *) &size))
    return -1;

  //printf ("Thread %s writing to fd %d %u bytes\n", thread_current ()->name, fd,
  //        size);
  written = 0;
  if (fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      written = size;
    }
    
  return written;
}

static
int sys_seek(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}

static
int sys_tell(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}

static
int sys_close(const uint8_t *arg_base)
{
  (void) arg_base;
  return -1;
}
