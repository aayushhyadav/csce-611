#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = nullptr;
ContFramePool * PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
}

PageTable::PageTable()
{
   Console::puts("\nPageTable::Setting up Paging\n");

   // setup the page directory
   page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

   // setup the page table
   unsigned long * page_table = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

   unsigned long address = 0, kernel_rw_present_mask = 3, kernel_rw_absent_mask = 2;
   unsigned int pte, pde;

   // direct map the first 4MB of memory
   for (pte = 0; pte < 1024; pte++) {
      page_table[pte] = address | kernel_rw_present_mask;
      address += 4096;
   }

   // populate the first entry in the page table directory
   page_directory[0] = *page_table;
   page_directory[0] |= kernel_rw_present_mask;

   // populate the remaining page directory entries
   for (pde = 1; pde < 1024; pde++) {
      page_directory[pde] = 0 | kernel_rw_absent_mask;
   }

   current_page_table = this;

   Console::puts("PageTable::Page Directory and Page Table setup correctly!\n");
}

void PageTable::load()
{
   // write the address of page directory in CR3 register
   write_cr3(*page_directory);
   Console::puts("\nPageTable::load loaded the page directory address in CR3 register\n");
}

void PageTable::enable_paging()
{
   // set the MSB of cr0 register to 1 to enable paging
   write_cr0(read_cr0() | 0x80000000);
   paging_enabled = 1;
   Console::puts("\nPageTable::enable_paging enabled paging by setting the MSB in CR0 register\n");
}

void PageTable::handle_fault(REGS * _r)
{
//   assert(false);
  Console::puts("handled page fault\n");
}

