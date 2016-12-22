#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/priority_queue.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Maximun number of sleeping threads at a tie */
#define THREAD_MAX 10000

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

/* Queue of sleeping threads */
static struct priority_queue *sleeping_queue;

/* Encapsulates the thread with its waking time to be pushed in sleeping_queue */
struct sleeping_thread
{
    struct thread *thread;    /* thread to wake */
    int64_t waking_time;      /* thread's waking time in clock ticks*/
};

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);
int sleeping_thread_compare(void *sleeping1, void *sleeping2);
void update_sleeping_threads(void);


/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void)
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
  //initialize Priority Queue of sleeping threads
  sleeping_queue = init_priority_queue(THREAD_MAX, sleeping_thread_compare);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void)
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1))
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (high_bit | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void)
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then)
{
  return timer_ticks () - then;
}

/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */int slept = 0, waked = 0;
void
timer_sleep (int64_t ticks)
{//ticks = ticks<0?ticks*-1:ticks;

  int64_t start = timer_ticks ();

  ASSERT (intr_get_level () == INTR_ON);

  /* Allocate memory for queue element */
  struct sleeping_thread* to_sleep =
            (struct sleeping_thread*) malloc(sizeof(struct sleeping_thread));

  struct thread *current_thread = thread_current();
  /* Initialize queue element */
  to_sleep->thread = current_thread;
  /* waking time = current time + delay */
  to_sleep->waking_time = start + ticks;

 /* disable interrupts */
  enum intr_level old_level;
  old_level = intr_disable ();

  //enter critical section
  push_priority_queue(sleeping_queue, (void*)to_sleep);

  /* Block the thread */
  thread_block();
  //leave critical section

  /* re enabling interrupts */
  intr_set_level (old_level);

}

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms)
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us)
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns)
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms)
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us)
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns)
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void)
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{//printf("+++++++++++interrupt\n");
  ticks++;
  //wake threads that finished sleeping time
  update_sleeping_threads();
  thread_tick ();
}

/* unblocks threads that finished sleeping time */
void update_sleeping_threads(void)
{
  //current time in ticks
  int64_t current_ticks = timer_ticks ();

  //lock_acquire(sleep_lock);

  while (size_priority_queue(sleeping_queue) > 0) {//we have blocked threads
    //get thread with most recent waking time
    struct sleeping_thread *sleeping_thread =
                              display_priority_queue(sleeping_queue);
    if (sleeping_thread->waking_time <= current_ticks) {//thread shlould be unblocked
      /* unblock the thread and remove it from
      the queue */
      pop_priority_queue(sleeping_queue);
      thread_unblock(sleeping_thread->thread);
    } else {
      //we are done
      break;
    }
  }
  //lock_release(sleep_lock);
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops)
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops)
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */
      timer_sleep (ticks);
    }
  else
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom);
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
}

/*
   Compares two structs of sleeping threads, and returns
   +ve value if first wakes later than second
   zero      if first wakes at the same time as second
   -ve value if first wakes before second
*/
int sleeping_thread_compare(void *sleeping1, void *sleeping2)
{
  struct sleeping_thread* sleeping_thread1 =
                                (struct sleeping_thread*) sleeping1;
  struct sleeping_thread* sleeping_thread2 =
                                (struct sleeping_thread*) sleeping2;


  int64_t temp = sleeping_thread2->waking_time - sleeping_thread1->waking_time;
  if (temp == 0) {//equal
    return sleeping_thread1->thread->priority
               - sleeping_thread2->thread->priority;
  }
  return (int) temp;
}
