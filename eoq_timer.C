#include "assert.H"
#include "utils.H"
#include "console.H"
#include "simple_timer.H"
#include "eoq_timer.H"
#include "scheduler.H"

/*--------------------------------------------------------------------------*/
/* EXTERNS */
/*--------------------------------------------------------------------------*/
extern RRScheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

EOQTimer::EOQTimer(int _hz): SimpleTimer(_hz) {
    seconds =  0;   /* How long has the system been running? */ 
    ticks   =  0;   /* ticks since last "seconds" update. */
    set_frequency(_hz);
}

void EOQTimer::set_frequency(int _hz) {
    hz = _hz;                                     /* Remember the frequency.           */
    int divisor = 1193180 / _hz;                  /* The input clock runs at 1.19MHz   */
    Machine::outportb(0x43, 0x34);                /* Set command byte to be 0x36.      */
    Machine::outportb(0x40, divisor & 0xFF);      /* Set low byte of divisor.          */
    Machine::outportb(0x40, divisor >> 8);        /* Set high byte of divisor.         */
}

void EOQTimer::handle_interrupt(REGS *_r) {
    /* Increment our "ticks" count */
    ticks++;
    
    /* time quantum has passed, preempt the current thread */
    if (ticks >= hz )
    {
        ticks = 0;
        seconds++;
        Console::puts("Time Quanta (50 ms) has passed \n");
        
        // invoke the scheduler
        SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        SYSTEM_SCHEDULER->yield();
    }
}