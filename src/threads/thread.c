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

#define THREAD_MAGIC 0x1234abcdu

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list run_queue;

/* Idle thread. */
static struct thread *idle_thread;      /* Thread. */
static void idle (void *aux UNUSED);    /* Thread function. */

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    void (*function) (void *);  /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

static void kernel_thread (void (*function) (void *aux), void *aux);

static struct thread *next_thread_to_run (void);
static struct thread *new_thread (const char *name);
static bool is_thread (struct thread *t);
static void *alloc_frame (struct thread *t, size_t size);
static void destroy_thread (struct thread *t);
static void schedule (void);
void schedule_tail (struct thread *prev);

/* Initializes the threading system.  After calling, create some
   threads with thread_create() or thread_execute(), then start
   the scheduler with thread_start(). */
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

/* Starts the thread scheduler.  The caller should have created
   some threads with thread_create() or thread_execute().  Never
   returns to the caller. */
void
thread_start (void) 
{
  struct thread *t = next_thread_to_run ();
  if (t->status == THREAD_READY)
    list_remove (&t->rq_elem);
  t->status = THREAD_RUNNING;
  switch_threads (NULL, t);

  NOT_REACHED ();
}

/* Creates a new kernel thread named NAME, which executes
   FUNCTION passing AUX as the argument, and adds it to the ready
   queue.  If thread_start() has been called, then the new thread
   may be scheduled before thread_create() returns.  Use a
   semaphore or some other form of synchronization if you need to
   ensure ordering. */
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
  thread_wake (t);

  return t;
}

#ifdef USERPROG
/* Starts a new thread running a user program loaded from
   FILENAME, and adds it to the ready queue.  If thread_start()
   has been called, then new thread may be scheduled before
   thread_execute() returns.*/
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
  thread_wake (t);

  return true;
}
#endif

/* Transitions T from its current state to THREAD_READY, the
   ready-to-run state.  On entry, T must be ready or blocked.
   (Use thread_yield() to make the running thread ready.) */
void
thread_wake (struct thread *t) 
{
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_READY || t->status == THREAD_BLOCKED);
  if (t->status != THREAD_READY) 
    {
      list_push_back (&run_queue, &t->rq_elem);
      t->status = THREAD_READY;
    }
}

/* Returns the name of thread T. */
const char *
thread_name (struct thread *t) 
{
  ASSERT (is_thread (t));
  return t->name;
}

/* Returns the running thread. */
struct thread *
thread_current (void) 
{
  uint32_t *esp;
  struct thread *t;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("movl %%esp, %0\n" : "=g" (esp));
  t = pg_round_down (esp);

  /* Make sure T is really a thread.
     If this assertion fires, then your thread may have
     overflowed its stack.  Each thread has less than 4 kB of
     stack, so a few big automatic arrays or moderate recursion
     can cause stack overflow. */
  ASSERT (is_thread (t));

  return t;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

  intr_disable ();
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum if_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  list_push_back (&run_queue, &cur->rq_elem);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_wake(). */
void
thread_sleep (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == IF_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

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

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (void (*function) (void *aux), void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t) 
{
  return t != NULL && t->magic == THREAD_MAGIC;
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
      t->status = THREAD_BLOCKED;
      t->magic = THREAD_MAGIC;
    }
  
  return t;
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&run_queue))
    return idle_thread;
  else
    return list_entry (list_pop_front (&run_queue), struct thread, rq_elem);
}

/* Destroys T, which must be in the dying state and must not be
   the running thread. */
static void
destroy_thread (struct thread *t) 
{
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_DYING);
  ASSERT (t != thread_current ());

  palloc_free (t);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   After this function and its caller returns, the thread switch
   is complete. */
void
schedule_tail (struct thread *prev) 
{
  struct thread *cur = thread_current ();
  
  ASSERT (intr_get_level () == IF_OFF);

  cur->status = THREAD_RUNNING;
  if (prev != NULL && prev->status == THREAD_DYING) 
    destroy_thread (prev);

#ifdef USERPROG
  addrspace_activate (&cur->addrspace);
#endif
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it. */
static void
schedule (void) 
{
  struct thread *cur = thread_current ();
  struct thread *next = next_thread_to_run ();

  ASSERT (intr_get_level () == IF_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    {
      struct thread *prev = switch_threads (cur, next);
      schedule_tail (prev); 
    }
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
