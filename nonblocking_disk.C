/*
     File        : nonblocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"
#include "system.H"

bool NonBlockingDisk::is_disk_op_issued = false;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(unsigned int _size) 
  : SimpleDisk(_size) {
    queue_size = 0;
}

/*--------------------------------------------------------------------------*/
/* NON-BLOCKING DISK OPERATIONS */
/*--------------------------------------------------------------------------*/

Thread * NonBlockingDisk::resume_blocked_thread() {
  Thread * first_thread = blocked_queue.dequeue();
  queue_size--;
  return first_thread;
}

void NonBlockingDisk::wait_while_busy() {
  blocked_queue.enqueue(Thread::CurrentThread());

  Console::puts("\nNonBlocking::wait_while_busy blocking thread ");
  Console::puti(Thread::CurrentThread()->ThreadId());
  Console::puts("\n");
  
  queue_size++;
  System::SCHEDULER->yield();   // context switch to the next thread to avoid busy waiting
}

void NonBlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // if another thread has already requested I/O wait until the request is serviced (thread safe)
  while (is_disk_op_issued) {
    System::SCHEDULER->resume(Thread::CurrentThread());   // add the current thread requesting I/O to the ready queue
    System::SCHEDULER->yield();   // context switch to the next thread to avoid busy waiting
  }

  is_disk_op_issued = true;
  SimpleDisk::read(_block_no, _buf);
  is_disk_op_issued = false;
}

void NonBlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // if another thread has already requested I/O wait until the request is serviced (thread safe)
  while (is_disk_op_issued) {
    System::SCHEDULER->resume(Thread::CurrentThread());   // add the current thread requesting I/O to the ready queue
    System::SCHEDULER->yield();   // context switch to the next thread to avoid busy waiting
  }

  is_disk_op_issued = true;
  SimpleDisk::write(_block_no, _buf);
  is_disk_op_issued = false;
}

bool NonBlockingDisk::schedule_blocked_thread() {
  return (!SimpleDisk::is_busy() && queue_size > 0);
}