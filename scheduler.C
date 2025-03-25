/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  queue_size = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  // disable the interrupts while manipulating the ready queue
  if (Machine::interrupts_enabled()) {
    Machine::disable_interrupts();
  }

  if (queue_size == 0) {
    Console::puts("Scheduler::yield ready queue is empty!\n");
    return;

  } else {
    // fetch the thread in FIFO order
    Thread * thread_to_schedule = ready_queue.dequeue();
    queue_size--;

    // enable the interrupts once ready queue has been manipulated
    if (!Machine::interrupts_enabled()) {
      Machine::enable_interrupts();
    }

    // context switch to the selected thread
    Thread::dispatch_to(thread_to_schedule);
  }
}

void Scheduler::resume(Thread * _thread) {
  // disable the interrupts while manipulating the ready queue
  if (Machine::interrupts_enabled()) {
    Machine::disable_interrupts();
  }

  // add the thread in the ready queue
  ready_queue.enqueue(_thread);
  queue_size++;

  // enable the interrupts once ready queue has been manipulated
  if (!Machine::interrupts_enabled()) {
    Machine::enable_interrupts();
  }

  Console::puts("Successfully added to the ready queue thread with ID - ");
  Console::puti(_thread->ThreadId());
  Console::puts("\n");
}

void Scheduler::add(Thread * _thread) {
  // simply invoke the resume function
  // to add the new thread in the ready queue
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  assert(false);
}
