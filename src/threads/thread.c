#include "thread.h"
#include <stddef.h>
#include "debug.h"
#include "interrupt.h"
#include "intr-stubs.h"
#include "lib.h"
#include "mmu.h"
#include "palloc.h"
#include "random.h"
#include "switch.h"

uint32_t thread_stack_ofs = offsetof (struct thread, stack);

static struct list run_queue;

void
thread_init (void) 
{
  list_init (&run_queue);
}

struct thread_root_frame 
  {
    void *eip;                  /* Return address. */
    void (*function) (void *);  /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

static void
thread_root (void (*function) (void *aux), void *aux) 
{
  ASSERT (function != NULL);
  
  function (aux);
  thread_exit ();
}

static struct thread *
new_thread (const char *name) 
{
  struct thread *t;

  ASSERT (name != NULL);
  
  t = palloc_get (PAL_ZERO);
  if (t != NULL)
    {
      strlcpy (t->name, name, sizeof t->name);
      t->stack = (uint8_t *) t + PGSIZE;
      t->status = THREAD_BLOCKED;
    }
  
  return t;
}

static void *
alloc_frame (struct thread *t, size_t size) 
{
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

struct thread *
thread_create (const char *name, void (*function) (void *aux), void *aux) 
{
  struct thread *t;
  struct thread_root_frame *rf;
  struct switch_thunk_frame *tf;
  struct switch_frame *sf;

  ASSERT (function != NULL);

  t = new_thread (name);

  /* Stack frame for thread_root(). */
  rf = alloc_frame (t, sizeof *rf);
  rf->eip = NULL;
  rf->function = function;
  rf->aux = aux;

  /* Stack frame for switch_thunk(). */
  tf = alloc_frame (t, sizeof *tf);
  tf->eip = (void (*) (void)) thread_root;

  /* Stack frame for thread_switch(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = (void (*) (void)) switch_thunk;

  /* Add to run queue. */
  thread_ready (t);

  return t;
}

struct thread *
thread_current (void) 
{
  uint32_t *esp;
  asm ("movl %%esp, %0\n" : "=g" (esp));
  return pg_round_down (esp);
}

#ifdef USERPROG
bool
thread_execute (const char *filename) 
{
  struct thread *t;
  struct intr_frame *if_;
  struct switch_thunk_frame *tf;
  struct switch_frame *sf;
  void (*start) (void);

  ASSERT (filename != NULL);

  t = new_thread (filename);
  if (t == NULL)
    return false;
  
  if (!addrspace_load (&t->addrspace, filename, &start)) 
    panic ("%s: program load failed", filename);

  /* Interrupt frame. */
  if_ = alloc_frame (t, sizeof *if_);
  if_->es = SEL_UDSEG;
  if_->ds = SEL_UDSEG;
  if_->eip = start;
  if_->cs = SEL_UCSEG;
  if_->eflags = FLAG_IF | 2;
  if_->esp = PHYS_BASE;
  if_->ss = SEL_UDSEG;
  
  /* Stack frame for switch_thunk(). */
  tf = alloc_frame (t, sizeof *tf);
  tf->eip = (void (*) (void)) intr_exit;

  /* Stack frame for thread_switch(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = (void (*) (void)) switch_thunk;

  /* Add to run queue. */
  thread_ready (t);

  return true;
}
#endif

void
thread_ready (struct thread *t) 
{
  if (t->status != THREAD_READY) 
    {
      list_push_back (&run_queue, &t->rq_elem);
      t->status = THREAD_READY;
    }
}

static struct thread *
find_next_to_run (void) 
{
  if (list_empty (&run_queue))
    return NULL;
  else
    return list_entry (list_pop_front (&run_queue), struct thread, rq_elem);
}

static void
idle (void) 
{
  static int idle = 0;
  if (idle++ == 0)
    printk ("idle\n");
}

void
thread_destroy (struct thread *t) 
{
  ASSERT (t->status == THREAD_DYING);
  ASSERT (t != thread_current ());

  palloc_free (t);
}

void
thread_schedule (void) 
{
  struct thread *cur, *next, *prev;

  ASSERT (intr_get_level () == IF_OFF);

  cur = thread_current ();
  ASSERT (cur->status != THREAD_RUNNING);

  while ((next = find_next_to_run ()) == NULL)
    idle ();

  next->status = THREAD_RUNNING;
  prev = switch_threads (cur, next);

  /* Prevent GCC from reordering anything around the thread
     switch. */
  asm volatile ("" : : : "memory");

#ifdef USERPROG
  addrspace_activate (&cur->addrspace);
#endif

  if (prev != NULL && prev->status == THREAD_DYING) 
    thread_destroy (prev);

  intr_enable ();
}

void
thread_yield (void) 
{
  ASSERT (!intr_context ());

  intr_disable ();
  thread_ready (thread_current ());
  thread_schedule ();
}

void
thread_start (struct thread *t) 
{
  ASSERT (intr_get_level () == IF_OFF);

  if (t->status == THREAD_READY) 
    list_remove (&t->rq_elem);
  t->status = THREAD_RUNNING;
  switch_threads (NULL, t);
}

void
thread_exit (void) 
{
  ASSERT (!intr_context ());

  intr_disable ();
  thread_current ()->status = THREAD_DYING;
  thread_schedule ();
}

void
thread_sleep (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == IF_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  thread_schedule ();
}

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

void
thread_self_test (void)
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
