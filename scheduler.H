/* 
    Author: R. Bettati, Joshua Capehart
            Department of Computer Science
            Texas A&M University
			
	    A thread scheduler.

*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* !!! IMPLEMENTATION HINT !!! */
/*--------------------------------------------------------------------------*/
/*
    One way to proceed is to implement the FIFO scheduling policy inside
    class 'Scheduler'. 

    If you plan to implement a Round-Robin Scheduler, derive it from class
    'Scheduler', say as class 'RRScheduler'. The class 'RRScheduler' is 
    really just a FIFO scheduler with THREE MODIFICATIONS:
    1. It manages a timer, which fires at the end-of-quantum (EOQ). 
    (For details on how to set up a timer and how to handle timer interrupts 
    see the 1-second timer in 'kernel.C'.)  The timer is set up in the
    constructor.
    2. It uses an additional function, the EOQ handler. This function gets
    called whenever an EOQ timer event fires. The EOQ handler forces the 
    current thread to call the scheduler's 'yield' function.
    3. The 'yield' function must be modified to account for unused quantum
    time. If a thread voluntarily yields, the EOQ timer must be reset in order
    to not penalize the next thread.
 
    (Note that this qualifies as programming at about the level of a baboon.
     Much better woudl be to have the abstract class 'Scheduler' implement 
     the basic scheduling MECHANISMS and provide abstract funtions to define
     the queue management POLICIES in derived classes, 
     such as 'FIFOScheduler'.)
    
 */

/*--------------------------------------------------------------------------*/
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

// maintains a pointer to the TCB
// and a pointer to the next TCB in the queue
struct Node {
   Thread * thread;
   struct Node * next;
};

// Queue class is used to implement the ready queue.
// The ready queue is implemented using a singly linked-list
class Queue {
   private:

      struct Node * head;  // points to the head for efficient dequeue operation
      struct Node * tail;  // points to the tail for efficient enqueue operation

   public:

      // initialize the queue
      Queue() {
         head = nullptr;
         tail = nullptr;
      }

      // append a new thread to the queue
      void enqueue(Thread * new_thread) {

         struct Node * new_node = new Node{new_thread, nullptr};

         // insert the first thread
         if (head == nullptr) {
            head = new_node;
            tail = head;
            return;
         }

         // append subsequent threads at the tail
         tail->next = new_node;
         tail = tail->next;
      }

      Thread * dequeue() {
         // queue is empty
         if (head == nullptr) return nullptr;

         struct Node * top_node = head;
         Thread * top_thread = top_node->thread;

         // queue has only one thread
         if (head->next == nullptr) {
            head = nullptr;
            tail = nullptr;

         // update the head
         } else {
            head = head->next;
            top_node->next = nullptr;
         }

         // free the memory occupied by struct node
         delete top_node;
         return top_thread;      
      }
};

class Scheduler {

  /* The scheduler may need private members... */
private:
   Queue ready_queue;
   int queue_size;
  
public:

   Scheduler();
   /* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */

   /* NOTE: We are making all functions virtual. This may come in handy when
            you want to derive RRScheduler from this class. */
  
   virtual void yield();
   /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

   virtual void resume(Thread * _thread);
   /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */

   virtual void add(Thread * _thread);
   /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */

   virtual void terminate(Thread * _thread);
   /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
  
};
	
/*--------------------------------------------------------------------------*/
/* ROUND ROBIN SCHEDULER */
/*--------------------------------------------------------------------------*/

class RRScheduler: public Scheduler {
   private:
      Queue ready_queue;
      int queue_size;

   public:
      RRScheduler();
      /*	Setup the Round-Robin scheduler */

      virtual void yield();
      /* Called by the currently running thread in order to give up the CPU. 
         The scheduler selects the next thread from the ready queue to load onto 
         the CPU, and calls the dispatcher function defined in 'Thread.H' to
         do the context switch. */

      virtual void resume(Thread * _thread);
      /* Add the given thread to the ready queue of the scheduler. This is called
         for threads that were waiting for an event to happen, or that have 
         to give up the CPU in response to a preemption. */

      virtual void add(Thread * _thread);
      /* Make the given thread runnable by the scheduler. This function is called
         after thread creation. Depending on implementation, this function may 
         just add the thread to the ready queue, using 'resume'. */

      virtual void terminate(Thread * _thread);
      /* Remove the given thread from the scheduler in preparation for destruction
         of the thread. 
         Graciously handle the case where the thread wants to terminate itself.*/
};

#endif
