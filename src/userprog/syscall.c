#include "syscall.h"
#include "lib.h"
#include "interrupt.h"
#include "thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  printk ("system call!\n");
  thread_exit ();
}
