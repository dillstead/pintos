#include "thread.h"
#include <stddef.h>
#include "debug.h"
#include "interrupt.h"
#include "lib.h"
#include "mmu.h"
#include "palloc.h"
#include "random.h"

uint32_t thread_stack_ofs = offsetof (struct thread, stack);

static struct list run_queue;

struct thread *thread_switch (struct thread *cur, struct thread *next);

void
thread_init (void) 
{
  list_init (&run_queue);
}

static void
thread_root (void (*function) (void *aux), void *aux) 
{
  ASSERT (function != NULL);
  
  function (aux);
  thread_exit ();
}

struct thread *
thread_create (const char *name, void (*function) (void *aux), void *aux) 
{
  struct thread *t;

  ASSERT (name != NULL);
  ASSERT (function != NULL);

  t = palloc_get (0);
  if (t == NULL)
    return NULL;

  memset (t, 0, PGSIZE);
  strlcpy (t->name, name, sizeof t->name);

  /* Set up stack. */
  t->stack = (uint32_t *) ((uint8_t *) t + PGSIZE);
  *--t->stack = (uint32_t) aux;
  *--t->stack = (uint32_t) function;
  --t->stack;
  *--t->stack = (uint32_t) thread_root;
  t->stack -= 4;

  /* Add to run_queue. */
  t->status = THREAD_BLOCKED;
  thread_ready (t);

  return t;
}

static struct thread *
stack_to_thread (uint32_t *stack) 
{
  return (struct thread *) ((uint32_t) (stack - 1) & ~((uint32_t) PGSIZE - 1));
}

struct thread *
thread_current (void) 
{
  uint32_t *esp;
  asm ("movl %%esp, %0\n" : "=g" (esp));
  return stack_to_thread (esp);
}

#ifdef USERPROG
void
thread_execute (const char *filename) 
{
  struct thread *t = thread_current ();
  
  if (!addrspace_load (&t->addrspace, filename)) 
    panic ("%s: program load failed", filename);
  panic ("%s: loaded", filename);
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
  prev = thread_switch (cur, next);

  /* Prevent GCC from reordering anything around the thread
     switch. */
  asm volatile ("" : : : "memory");

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
  thread_switch (NULL, t);
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
