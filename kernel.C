/*
 File: kernel.C
 
 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 2024/08/08
 
 This is the main kernel file. 
 This file has the main entry point to the operating system.
 
 */


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 32
/* Number of recursive allocations that we use to test.  */

#define FAULT_ADDR (4 MB)
/* used in the code later as address referenced to cause page faults. */
#define NACCESS ((1 MB) / 4)
/* NACCESS integer access (i.e. 4 bytes in each access) are made starting at address FAULT_ADDR */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H"  /* The physical memory manager */

#include "gdt.H"
#include "idt.H"          /* LOW-LEVEL EXCEPTION MGMT. */
#include "irq.H"
#include "exceptions.H"
#include "interrupts.H"

#include "simple_timer.H" /* TIMER MANAGEMENT */

#include "page_table.H"
#include "paging_low.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go);
void test_get_frames(ContFramePool * _pool, unsigned int pool_type);
void test_get_frames_utility(ContFramePool * _pool, unsigned int n_frames);
void test_release_frames(ContFramePool* _pool, unsigned int pool_type);

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {
    
    /* -- We initialize the global descriptor table and interrupt descriptor tables */
    GDT::init();
    Console::init();
    Console::redirect_output(true);

    IDT::init();
    ExceptionHandler::init_dispatcher();
    IRQ::init();
    InterruptHandler::init_dispatcher();
    
    
    /* -- EXAMPLE OF AN EXCEPTION HANDLER : Division-by-Zero -- */
    
    class DBZ_Handler : public ExceptionHandler {
      /* We derive Division-by-Zero handler from ExceptionHandler 
         and overload the method handle_exception. */
    public:
        virtual void handle_exception(REGS * _regs) {
            // The exception handler function simply throws a hissy fit.
            Console::puts("DIVISION BY ZERO!\n");
            for(;;);
        }
    } dbz_handler;
    
    /* Register the DBZ handler for exception no.0 
       with the exception dispatcher. */
    ExceptionHandler::register_handler(0, &dbz_handler);

    /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */

    /*    The SimpleTimer is derived from InterruptHandler 
          and is defined in file simple_timer.H/C. */
    SimpleTimer timer(100); /* timer ticks every 10ms. */
    
    /* ---- Because the SimpleTimer is derived from InterruptHandler, 
            we register the timer handler for interrupt no.0 
            with the interrupt dispatcher. */
    InterruptHandler::register_handler(0, &timer);
    
    /* NOTE: The timer chip starts periodically firing as 
             soon as we enable interrupts.
             It is important to install a timer handler, as we would 
             get a lot of uncaptured interrupts otherwise. */

    /* -- ENABLE INTERRUPTS -- */
    
    Machine::enable_interrupts();

    /* -- INITIALIZE FRAME POOLS -- */

    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0);

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);
    
    /* Take care of the hole in the memory. */
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
    
    /* -- INITIALIZE MEMORY (PAGING) -- */
    
    /* ---- INSTALL PAGE FAULT HANDLER -- */
   
    class PageFault_Handler : public ExceptionHandler {
        /* We derive the page fault handler from ExceptionHandler 
           and overload the method handle_exception. */
    public:
        virtual void handle_exception(REGS * _regs) {
            PageTable::handle_fault(_regs);
        }
    } pagefault_handler;
    
    /* ---- Register the page fault handler for exception no.14 
            with the exception dispatcher. */
    ExceptionHandler::register_handler(14, &pagefault_handler);
    
    /* ---- INITIALIZE THE PAGE TABLE -- */
    
    PageTable::init_paging(&kernel_mem_pool,
                           &process_mem_pool,
                           4 MB);
    
    PageTable pt;
    
    pt.load();

    PageTable::enable_paging();

    Console::puts("WE TURNED ON PAGING!\n");
    Console::puts("If we see this message, the page tables have been\n");
    Console::puts("set up mostly correctly.\n");

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */
    
    /* -- TEST MEMORY ALLOCATOR */
    
    Console::puts("\n---Testing the Kernel Memory Allocator (Provided Test Function)---\n\n");
    test_memory(&kernel_mem_pool, N_TEST_ALLOCATIONS);

    test_get_frames(&kernel_mem_pool, 0);
    test_get_frames(&process_mem_pool, 1);

    test_release_frames(&kernel_mem_pool, 0);
    test_release_frames(&process_mem_pool, 1);

    /* -- GENERATE MEMORY REFERENCES */
    
    int *foo = (int *) FAULT_ADDR;
    int i;

    for (i=0; i<NACCESS; i++) {
        foo[i] = i;
    }

    Console::puts("DONE WRITING TO MEMORY. Now testing...\n");

    for (i=0; i<NACCESS; i++) {
        if(foo[i] != i) { 
            // The value in the memory location is different than the value that we wrote earlier.
            Console::puts("TEST FAILED for access number:");
            Console::putui(i);
            Console::puts("\n");
            break;
        }
    }
    if(i == NACCESS) {
        Console::puts("TEST PASSED\n");
    }

    /* -- STOP HERE */
    Console::puts("YOU CAN SAFELY TURN OFF THE MACHINE NOW.\n");
    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go) {
    Console::puts("alloc_to_go = "); Console::puti(_allocs_to_go); Console::puts("\n");
    if (_allocs_to_go > 0) {
        // We have not reached the end yet. 
        int n_frames = _allocs_to_go % 4 + 1;               // number of frames you want to allocate
        unsigned long frame = _pool->get_frames(n_frames);  // we allocate the frames from the pool
        int * value_array = (int*)(frame * (4 KB));         // we pick a unique number that we want to write into the memory we just allocated
        for (int i = 0; i < (1 KB) * n_frames; i++) {       // we write this value int the memory locations
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1);              // recursively allocate and uniquely mark more memory
        for (int i = 0; i < (1 KB) * n_frames; i++) {       // We check the values written into the memory before we recursed 
            if(value_array[i] != _allocs_to_go){            // If the value stored in the memory locations is not the same that we wrote a few lines above
                                                            // then somebody overwrote the memory.
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i ="); Console::puti(i);
                Console::puts("   v = "); Console::puti(value_array[i]); 
                Console::puts("   n ="); Console::puti(_allocs_to_go);
                Console::puts("\n");
                for(;;);                                    // We throw a fit.
            }
        }
        ContFramePool::release_frames(frame);               // We free the memory that we allocated above.
    }
}

void test_get_frames_utility(ContFramePool* _pool, unsigned int n_frames) {
    unsigned long frame = _pool->get_frames(n_frames);

    if (n_frames <= 511) {
        if(frame == 0) {
            Console::puts("Test Case Failed!\n\n");
            return;
        }
        ContFramePool::release_frames(frame);
        Console::puts("Test Case Passed!\n\n");

    } else {
        if (frame != 0) {
            Console::puts("Test Case Failed!\n\n");
            assert(false);
        }
        Console::puts("Test Case Passed!\n\n");
    }
}

void test_get_frames(ContFramePool* _pool, unsigned int pool_type) {
    if (pool_type == 0) {
        Console::puts("\n---Testing the Kernel Memory Allocator (Allocating 500 frames at a time)---\n\n");
        test_get_frames_utility(_pool, 500);

        Console::puts("\n---Testing the Kernel Memory Allocator (Allocating 1000 frames at a time)---\n\n");
        test_get_frames_utility(_pool, 1000);

    } else {
        Console::puts("\n---Testing the Process Memory Allocator (External Fragmentation Scenario)---\n\n");
        test_get_frames_utility(_pool, 6000);
    }
}

void test_release_frames(ContFramePool* _pool, unsigned int pool_type) {
    if (pool_type == 0) {
        Console::puts("\n---Testing the Kernel Memory Allocator (Relasing a frame which is not HoS)---\n\n");
        ContFramePool::release_frames(600);

    } else {
        Console::puts("\n---Testing the Process Memory Allocator (Releasing a frame managed by Process Pool)---\n\n");
        unsigned long frame = _pool->get_frames(100);
        ContFramePool::release_frames(frame);
    }
}

