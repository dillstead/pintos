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

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list run_queue;

/* Thread to run when nothing else is ready. */
static struct thread *idle_thread;

static struct thread *find_next_to_run (void);

/* Idle thread.  Executes when no other thread is ready to run. */
static void
idle (void *aux UNUSED) 
{
  for (;;) 
    {
      /* Wait for an interrupt. */
      DEBUG (idle, "idle");
      asm ("hlt");

      /* Let someone else run. */
      intr_disable ();
      thread_sleep ();
      intr_enable ();
    }
}

/* Initializes the threading system and starts an initial thread
   which is immediately scheduled.  Never returns to the caller.
   The initial thread is named NAME and executes FUNCTION passing
   AUX as the argument. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == IF_OFF);

  /* Initialize run queue. */
  list_init (&run_queue);

  /* Create idle thread. */
  idle_thread = thread_create ("idle", idle, NULL);
  idle_thread->status = THREAD_BLOCKED;
}

void
thread_start (void) 
{
  struct thread *t = find_next_to_run ();
  if (t->status == THREAD_READY)
    list_remove (&t->rq_elem);
  t->status = THREAD_RUNNING;
  switch_threads (NULL, t);

  NOT_REACHED ();
}

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    void (*function) (void *);  /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (void (*function) (void *aux), void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Creates a new thread named NAME and initializes its fields.
   Returns the new thread if successful or a null pointer on
   failure. */
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
      t->status = THREAD_INITIALIZING;
    }
  
  return t;
}

/* Allocates a SIZE-byte frame within thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Creates a new kernel thread named NAME, which executes
   FUNCTION passing AUX as the argument.  The thread is added to
   the ready queue.  Thus, it may be scheduled even before
   thread_create() returns.  If you need to ensure ordering, then
   use synchronization, such as a semaphore. */
struct thread *
thread_create (const char *name, void (*function) (void *aux), void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;

  ASSERT (function != NULL);

  t = new_thread (name);

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;

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
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  void (*start) (void);

  ASSERT (filename != NULL);

  t = new_thread (filename);
  if (t == NULL)
    return false;
  
  if (!addrspace_load (&t->addrspace, filename, &start)) 
    PANIC ("%s: program load failed", filename);

  /* Interrupt frame. */
  if_ = alloc_frame (t, sizeof *if_);
  if_->es = SEL_UDSEG;
  if_->ds = SEL_UDSEG;
  if_->eip = start;
  if_->cs = SEL_UCSEG;
  if_->eflags = FLAG_IF | 2;
  if_->esp = PHYS_BASE;
  if_->ss = SEL_UDSEG;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = intr_exit;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;

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
    return idle_thread;
  else
    return list_entry (list_pop_front (&run_queue), struct thread, rq_elem);
}

void
thread_destroy (struct thread *t) 
{
  ASSERT (t->status == THREAD_DYING);
  ASSERT (t != thread_current ());

  palloc_free (t);
}

void schedule_tail (struct thread *prev);

void
schedule_tail (struct thread *prev) 
{
  ASSERT (intr_get_level () == IF_OFF);

#ifdef USERPROG
  addrspace_activate (&thread_current ()->addrspace);
#endif

  if (prev != NULL && prev->status == THREAD_DYING) 
    thread_destroy (prev);
}

static void
thread_schedule (void) 
{
  struct thread *cur, *next, *prev;

  ASSERT (intr_get_level () == IF_OFF);

  cur = thread_current ();
  ASSERT (cur->status != THREAD_RUNNING);

  next = find_next_to_run ();

  next->status = THREAD_RUNNING;
  if (cur != next)
    {
      prev = switch_threads (cur, next);

      /* Prevent GCC from reordering anything around the thread
         switch. */
      asm volatile ("" : : : "memory");

      schedule_tail (prev); 
    }
}

void
thread_yield (void) 
{
  enum if_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  thread_ready (thread_current ());
  thread_schedule ();
  intr_set_level (old_level);
}

void
thread_exit (void) 
{
  ASSERT (!intr_context ());

  intr_disable ();
  thread_current ()->status = THREAD_DYING;
  thread_schedule ();
  NOT_REACHED ();
}

void
thread_sleep (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == IF_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  thread_schedule ();
}
