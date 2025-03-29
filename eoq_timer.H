#ifndef _EOQ_TIMER_H_
#define _EOQ_TIMER_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "simple_timer.H"

/*--------------------------------------------------------------------------*/
/* E O Q  T I M E R  */
/*--------------------------------------------------------------------------*/

class EOQTimer: public SimpleTimer {

private:

  unsigned long seconds; /* How long has the system been running? */
  int           ticks;   /* ticks since last "seconds" update.    */
  int           hz;      /* At what frequency do we update the ticks counter? */

  void set_frequency(int _hz);  /* Set the interrupt frequency for the simple timer. */

public :

  EOQTimer(int _hz);
  /* Initialize the simple timer, and set its frequency. */

  virtual void handle_interrupt(REGS *_r);
  /* This must be installed as the interrupt handler for the timer 
     when the system gets initialized. (e.g. in "kernel.C")  
  */

};

#endif
