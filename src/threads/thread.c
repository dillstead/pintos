#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/vaddr.h"
#include "threads/fixed-point.h"
#include "devices/timer.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* If a lock is waiting on a lock which is waiting on a lock...
   don't go further than PRI_MAX_DONATION_NESTING when
   donating priorities. */
#define PRI_MAX_DONATION_NESTING 8

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. The list
   is maintained in sorted order by priority with the process 
   of highest priority at the beginning of the list.*/
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
{
  void *eip;                  /* Return address. */
  thread_func *function;      /* Function to call. */
  void *aux;                  /* Auxiliary data for function. */
};

/* Sleeping. */
struct sleeping_thread
{
  struct list_elem elem;
  struct thread *thread;
  /* The number of ticks from the previous entry (if any) in the list to wakeup after. */
  int64_t wakeup;  
};

/* List of threads that are asleep. */
static struct list sleep_list;

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* Used for advanced scheduler. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

/* Recalculate thread priorities every PRIORITY_FREQ ticks. */
#define PRIORITY_FREQ 4

/* A moving average of the number of threads ready to run. */
static int load_avg;

static void update_priority_and_cpu (struct thread *t, void *aux);
static void update_priority (struct thread *t);

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);
static void shuffle_ready_thread (struct thread *thread);
static int trim_priority (int priority);
static bool maybe_raise_priority (struct thread *thread, int priority);
static void maybe_lower_priority (struct thread *thread, int priority);
static bool thread_priority_compare (const struct list_elem *a, const struct list_elem *b,
                                     void *aux UNUSED);
static void maybe_yield_to_ready_thread (void);
static bool lock_in_thread_locks_owned_list (struct lock *lock);
/* Used to keep the list of sleeping threads in correct order. */
static bool sleepers_tick_compare (const struct list_elem *a, const struct list_elem *b,
                                   void *aux UNUSED);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);
  list_init (&sleep_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  
  /* Used for advanced scheduler. */
  initial_thread->nice = NICE_DEFAULT;
  initial_thread->recent_cpu = 0;
  
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();
  struct sleeping_thread *sleeping_thread;
  struct list_elem *e;
  bool is_idle_thread;
  
  /* Used for the advanced scheduler. */
  int ready_threads;
  bool update_load_and_cpu;

  is_idle_thread = (t == idle_thread);
  
  /* Update statistics. */
  if (is_idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  if (thread_mlfqs)
    {
      if (!is_idle_thread)
        /* Update recent CPU of the current thread on every tick. */
        t->recent_cpu += INT_TO_FP(1);
      if (timer_ticks () % PRIORITY_FREQ == 0)
        {
          update_load_and_cpu = (timer_ticks () % TIMER_FREQ == 0);
          if (update_load_and_cpu)
            {
              /* Update the load average once per second using:
                 load_avg = (59 / 60) * load_avg + (1 / 60) * ready_threads
                 where ready_threads is the number of threads that are either
                 running or ready to run at time of update 
                 (not including the idle thread). */
              ready_threads = is_idle_thread ? 0 : 1;
              ready_threads += list_size (&ready_list);
              load_avg = FP_MUL(INT_TO_FP(59) / 60, load_avg)
                + INT_TO_FP(1) / 60 * ready_threads;
            }
          thread_foreach (update_priority_and_cpu,
                          (void *) &update_load_and_cpu);
          if (update_load_and_cpu)
            list_sort (&ready_list, thread_priority_compare, NULL);
          intr_yield_on_return ();
        }
    }
  else if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();

  /* Wakeup any sleeping threads. */
  if (!list_empty (&sleep_list))
    {
      sleeping_thread = list_entry (list_front (&sleep_list),
                                    struct sleeping_thread, elem);
      sleeping_thread->wakeup -= 1;
      for (e = list_begin (&sleep_list); e != list_end (&sleep_list); )
        {
          sleeping_thread = list_entry (e, struct sleeping_thread, elem);
          if (sleeping_thread->wakeup > 0)
            break;
          e = list_remove (e);
          thread_unblock (sleeping_thread->thread);
        }
    }
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code providned sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  struct thread *cur = thread_current ();
  enum intr_level old_level;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

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
  sf->ebp = 0;

  /* Used for advanced scheduler. */
  if (thread_mlfqs)
    {
      t->nice = cur->nice;      
      old_level = intr_disable ();
      t->recent_cpu = cur->recent_cpu;
      t->priority = cur->priority;
      intr_set_level (old_level);
    }

  /* Add to run queue. */
  thread_unblock (t);

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Puts the thread to sleep to be woken up after ticks clock ticks. */
void
thread_sleep (int64_t ticks)
{
  struct sleeping_thread to_sleep_thread;
  
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  if (ticks > 0)
    {
      to_sleep_thread.thread = thread_current ();
      to_sleep_thread.wakeup = ticks;
      list_insert_ordered (&sleep_list, &to_sleep_thread.elem,
                           sleepers_tick_compare, NULL);
      thread_block();
    }
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function may preempt the running thread if the thread
   being unblocked has an effective priority greater than the
   running thread. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);

  list_insert_ordered (&ready_list, &t->elem, thread_priority_compare,
                       NULL);
  t->status = THREAD_READY;      

  maybe_yield_to_ready_thread ();
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  struct thread *cur = thread_current();
  
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
  ASSERT (intr_get_level () == INTR_OFF);
#else
  intr_disable ();
  cur->status = THREAD_DYING;
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail() but only if our parent
     has already been destroyed and can't wait for us. */
  list_remove (&cur->allelem);
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread)
    list_insert_ordered (&ready_list, &cur->elem, thread_priority_compare,
                         NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's base priority to NEW_PRIORITY.
   The thread's effective priority will be set if new_priority
   is higher than the effective priority or if priority 
   donation is not in effect.  

   This function may preempt the running thread if the effective
   priority is lowered below that of the highest priority ready 
   thread. */
void
thread_set_priority (int new_priority) 
{
  enum intr_level old_level;

  new_priority = trim_priority (new_priority);  
  old_level = intr_disable ();
  if (thread_current ()->base_priority == thread_current ()->priority
      || new_priority > thread_current ()->priority)
    thread_current ()->priority = new_priority;      
  thread_current ()->base_priority = new_priority;
  maybe_yield_to_ready_thread ();
  intr_set_level (old_level);
}

/* Returns the current thread's effective priority. */
int
thread_get_priority (void) 
{
  enum intr_level old_level;
  int priority;

  old_level = intr_disable ();
  priority = thread_current ()->priority;
  intr_set_level (old_level);

  return priority;
}

/* Called after a thread has acquired a lock, adds the lock to the list
   of locks owned by the thread. */
void
thread_lock_acquired (struct lock *lock)
{
  if (thread_mlfqs)
    return;
  
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (!intr_context ());
  ASSERT (lock != NULL);
  ASSERT (lock_get_holder (lock) == thread_current());
  ASSERT (!lock_in_thread_locks_owned_list (lock));

  /* Push to front as locks are usually released in reverse
     order of acquisition. */
  list_push_front (&thread_current ()->locks_owned_list, &lock->elem);
  thread_current ()->waiting_lock = NULL;
}

/* Called when a thread will wait on a lock.  If the effective priority of the
   thread is higher than that of the lock owner, the higher priority will be 
   donated to the owner and so on. */
void
thread_lock_will_wait (struct lock *lock)
{    
  struct thread *thread;
  int nesting;

  if (thread_mlfqs)
    return;
  
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (!intr_context ());
  ASSERT (lock != NULL);
  ASSERT (lock_get_holder (lock) != thread_current());
  ASSERT (!lock_in_thread_locks_owned_list (lock));

  thread = lock_get_holder (lock);
  nesting = 0;

  while (thread != NULL && nesting < PRI_MAX_DONATION_NESTING)
    {
      if (thread->status == THREAD_BLOCKED)
        {
          maybe_raise_priority (thread, thread_current ()->priority);
          if (thread->waiting_lock != NULL)
            thread = lock_get_holder (thread->waiting_lock);
          else
            thread = NULL;
        }
      else
        {
          if (thread->status == THREAD_READY
              && maybe_raise_priority (thread, thread_current ()->priority))
            shuffle_ready_thread (thread);
          thread = NULL;
        }
      nesting++;
    }
  thread_current ()->waiting_lock = lock;
}

/* Called after a thread has released a lock.  The threads effective priority 
   will be set to the highest priority waiting thread on the remaining locks 
   that it owns.

   This function may preempt the running thread if the effective priority 
   is lowered below that of the highest priority ready thread. */
void
thread_lock_released (struct lock *lock)
{
  struct list *locks_owned_list;
  struct list_elem *e;
  struct lock *owned_lock;
  struct thread *waiting_thread;
  int highest_waiting_priority;

  if (thread_mlfqs)
    return;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (lock != NULL);
  ASSERT (lock_get_holder (lock) != thread_current());
  ASSERT (lock_in_thread_locks_owned_list (lock));

  highest_waiting_priority = PRI_MIN;
  locks_owned_list = &thread_current ()->locks_owned_list;
  for (e = list_begin (locks_owned_list); e != list_end (locks_owned_list); )
    {
      owned_lock = list_entry (e, struct lock, elem);
      if (lock == owned_lock)
        e = list_remove (e);
      else
        {
          waiting_thread = lock_get_highest_priority_waiting_thread (owned_lock);
          if (waiting_thread != NULL
              && waiting_thread->priority > highest_waiting_priority)
            highest_waiting_priority = waiting_thread->priority;
          e = list_next (e);
        }
    }
  maybe_lower_priority (thread_current(), highest_waiting_priority);
  maybe_yield_to_ready_thread ();
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice) 
{
  enum intr_level old_level;

  if (nice > NICE_MAX)
    nice = NICE_MAX;
  if (nice < NICE_MIN)
    nice = NICE_MIN;
    
  old_level = intr_disable ();
  thread_current ()->nice = nice;
  update_priority (thread_current ());
  maybe_yield_to_ready_thread ();
  intr_set_level (old_level);
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  return thread_current ()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  enum intr_level old_level;
  int current_load_avg;
    
  old_level = intr_disable ();
  current_load_avg = FP_TO_NEAREST_INT(load_avg * 100);
  intr_set_level (old_level);

  return current_load_avg;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  enum intr_level old_level;
  int current_recent_cpu;
    
  old_level = intr_disable ();
  current_recent_cpu = FP_TO_NEAREST_INT(thread_current ()->recent_cpu * 100);
  intr_set_level (old_level);

  return current_recent_cpu;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Functions used for advanced scheduler. */

static void
update_priority_and_cpu(struct thread *t, void *aux)
{
  bool update_cpu  = *((bool *) aux);

  if (t == idle_thread)
    return;
  
  if (update_cpu)
    /* Update recent cpu per second using:
       recent_cpu = (2 * load_avg) / (2 * load_avg + 1) * recent_cpu + nice
       where load_avg is a moving average of the number of threads ready to
       run. */
    t->recent_cpu = FP_MUL(FP_DIV(load_avg * 2, load_avg * 2 + INT_TO_FP(1)),
                           t->recent_cpu) + INT_TO_FP(t->nice);
  update_priority (t);
}

static void
update_priority (struct thread *t)
{
  /* Update priority every PRIORITY_FREQ ticks using:
     priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
     where recent_cpu is an estimate of the CPU time the 
     thread has used recently. */
  t->priority = trim_priority (FP_TO_INT(INT_TO_FP(PRI_MAX)
                                         - t->recent_cpu / 4
                                         - INT_TO_FP(t->nice * 2)));
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->base_priority = priority;
  t->priority = priority;
  t->magic = THREAD_MAGIC;
  list_init (&t->locks_owned_list);
#ifdef USERPROG
  t->exit_status = -1;
  list_init (&t->child_list);
  lock_init (&t->exit_lock);
  cond_init (&t->exiting);
#endif

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
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
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Shuffles a ready thread's position in the ready list after a priority 
   donation. */
static void
shuffle_ready_thread (struct thread *thread)
{
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (!intr_context ());
  ASSERT (thread->status == THREAD_READY);

  list_remove(&thread->elem);
  list_insert_ordered (&ready_list, &thread->elem, thread_priority_compare,
                       NULL);
}

static int
trim_priority (int priority)
{
  if (priority > PRI_MAX)
    return PRI_MAX;
  else if (priority < PRI_MIN)
    return PRI_MIN;
  else
    return priority;
}

/* Returns true if thread's effective priority was raised to priority, false 
   if not. */
static bool 
maybe_raise_priority (struct thread *thread, int priority)
{
  if (priority > thread->priority)
    {
      thread->priority = priority;
      return true;
    }
  return false;
}

/* Lowers the thread's effective priority to priority, but no lower than 
   the base priority. */
static void
maybe_lower_priority (struct thread *thread, int priority)
{
  ASSERT (priority <= thread->priority);
  
  if (priority < thread->base_priority)
    thread->priority = thread->base_priority;
  else
    thread->priority = priority;
}

/* Used to keep the ready list in effective priority order. */
static bool
thread_priority_compare (const struct list_elem *a, const struct list_elem *b,
                         void *aux UNUSED)
{
  return list_entry (a, struct thread, elem)->priority
    > list_entry (b, struct thread, elem)->priority;
}

/* If the thread's effective priority has dropped below that of the highest
   priority waiting thread, yield the CPU. */
static void
maybe_yield_to_ready_thread (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

  if (!list_empty (&ready_list)
      && thread_current ()->priority
      < list_entry (list_front (&ready_list), struct thread, elem)->priority)
    {
      if (intr_context ())
        intr_yield_on_return ();
      else
        thread_yield ();
    }
}

static bool
lock_in_thread_locks_owned_list (struct lock *lock)
{
  struct list *locks_list;
  struct list_elem *e;

  locks_list = &thread_current ()->locks_owned_list;
  for (e = list_begin (locks_list); e != list_end (locks_list);
       e = list_next (e))
    if (lock == list_entry (e, struct lock, elem))
      return true;
  
  return false;
}

static bool
sleepers_tick_compare (const struct list_elem *a, const struct list_elem *b,
                       void *aux UNUSED)
{
  struct sleeping_thread *to_sleep_thread;
  struct sleeping_thread *sleeping_thread;
    
  to_sleep_thread = list_entry (a, struct sleeping_thread, elem);
  sleeping_thread = list_entry (b, struct sleeping_thread, elem);
  
  if (to_sleep_thread->wakeup < sleeping_thread->wakeup)
    {
      sleeping_thread->wakeup -= to_sleep_thread->wakeup;
      return true;
    }
  else
    {
      to_sleep_thread->wakeup -= sleeping_thread->wakeup;
      return false;
    }
}
                                   

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
