#include "userprog/syscall.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

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

  if (!is_user_vaddr (uaddr))
      return -1;
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

  if (!is_user_vaddr (udst))
      return false;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
      
  return error_code != -1;
}

static bool
read_int (const uint8_t *uaddr, int *pi)
{
  int b0, b1, b2, b3;

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

/* Gets an integer argument at the specified positon from user space. */
static bool
get_int_arg (const uint8_t *uaddr, int pos, int *pi)
{
  return read_int (uaddr + sizeof (int) * pos, pi);
}

/* Gets a string argument at the specified position from user space. */
static bool
get_str_arg (const uint8_t *uaddr, int pos, char **pstr)
{
  uint8_t *ustraddr;
  int b;

  if (!get_int_arg (uaddr, pos, (int *) pstr))
    return false;
  ustraddr = (uint8_t *) *pstr;

  /* Verify all of the bytes in the string and ensure that it's
     properly terminated. */
  b = get_user (ustraddr);
  while (b != -1)
    {
      if (b == 0)
        return true;
      b = get_user (ustraddr++);
    }
  
  return false;
}

static void
syscall_handler (struct intr_frame *f) 
{
  unsigned num;

  if (!get_int_arg (f->esp, 0, (int *) &num))
      thread_exit ();
  if (num > 0 && num < sizeof syscalls / sizeof *syscalls
      && syscalls[num] != NULL)
    f->eax = syscalls[num] ((uint8_t *) f->esp + sizeof (int));
  else
    f->eax = -1;
}

static int
sys_halt (const uint8_t *arg_base)
{
  (void) arg_base;

  shutdown_power_off ();
  NOT_REACHED ();
  
  return 0;
}

static int
sys_exit (const uint8_t *arg_base)
{
  int status;

  if (!get_int_arg (arg_base, 0, &status))
    thread_exit ();
  thread_current ()->exit_status = status;
  thread_exit ();
  NOT_REACHED ();

  return 0;
}

static int
sys_exec (const uint8_t *arg_base)
{
  char *cmd_line;
  
  if (!get_str_arg (arg_base, 0, &cmd_line))
    thread_exit ();
  
  return process_execute (cmd_line);
}

static int
sys_wait (const uint8_t *arg_base)
{
  tid_t child_tid;

  if (!get_int_arg (arg_base, 0, (int *) &child_tid))
      thread_exit ();
  
  return process_wait (child_tid);
}

static int
sys_create (const uint8_t *arg_base)
{
  char *name;
  off_t initial_size;

  if (!get_str_arg (arg_base, 0, &name)
      || !get_int_arg (arg_base, 1, (int *) &initial_size))
    thread_exit ();
  
  return process_file_create (name, initial_size);
}

static int
sys_remove (const uint8_t *arg_base)
{
  char *name;

  if (!get_str_arg (arg_base, 0, &name))
    thread_exit ();
  
  return process_file_remove (name);
}

static int
sys_open (const uint8_t *arg_base)
{
  char *name;

  if (!get_str_arg (arg_base, 0, &name))
    thread_exit ();
  
  return process_file_open (name, false);  
}

static int
sys_filesize (const uint8_t *arg_base)
{
  int fd;
  
  if (!get_int_arg (arg_base, 0, &fd))
    thread_exit ();
  
  return process_file_size (fd);
}

static int
sys_read (const uint8_t *arg_base)
{
  int fd;
  void *buffer;
  off_t size;
  int b0, b1;
  
  if (!get_int_arg (arg_base, 0, &fd)
      || !get_int_arg (arg_base, 1, (int *) &buffer)
      || !get_int_arg (arg_base, 2, (int *) &size)
      || (b0 = get_user (buffer)) == -1
      || (b1 = get_user (buffer +  size)) == -1
      || !put_user (buffer, b0)
      || !put_user (buffer + size, b1))
    thread_exit ();
  
  return process_file_read (fd, buffer, size);
}

static int
sys_write (const uint8_t *arg_base)
{
  int fd;
  const void *buffer;
  off_t size;
  
  if (!get_int_arg (arg_base, 0, &fd)
      || !get_int_arg (arg_base, 1, (int *) &buffer)
      || !get_int_arg (arg_base, 2, (int *) &size)
      || get_user (buffer) == -1
      || get_user (buffer + size) == -1)
    thread_exit ();
  
  return process_file_write (fd, buffer, size);
}

static int
sys_seek (const uint8_t *arg_base)
{
  int fd;
  off_t new_pos;
  
  if (!get_int_arg (arg_base, 0, &fd)
      || !get_int_arg (arg_base, 1, (int *) &new_pos))
    thread_exit ();
  process_file_seek (fd, new_pos);
  
  return 0;
}

static int
sys_tell (const uint8_t *arg_base)
{
  int fd;
  
  if (!get_int_arg (arg_base, 0, &fd))
    thread_exit ();
  
  return process_file_tell (fd);
}

static int
sys_close (const uint8_t *arg_base)
{
  int fd;

  if (!get_int_arg (arg_base, 0, &fd))
    thread_exit ();
  process_file_close (fd);

  return 0;
}
