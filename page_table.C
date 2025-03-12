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
   for (pte = 0; pte < ENTRIES_PER_PAGE; pte++) {
      page_table[pte] = address | kernel_rw_present_mask;
      address += PAGE_SIZE;
   }

   // populate the first entry in the page table directory
   page_directory[0] = (unsigned long) page_table;
   page_directory[0] |= kernel_rw_present_mask;

   // populate the remaining page directory entries
   for (pde = 1; pde < ENTRIES_PER_PAGE; pde++) {
      page_directory[pde] = 0 | kernel_rw_absent_mask;
   }

   Console::puts("PageTable::Page Directory and Page Table setup correctly!\n\n");
}


void PageTable::load()
{
   current_page_table = this;

   // write the address of page directory in CR3 register
   write_cr3((unsigned long) current_page_table->page_directory);

   Console::puts("\nPageTable::load loaded the page directory address in CR3 register\n");
}

void PageTable::enable_paging()
{
   // set bit 31 of CR0 register to 1 to enable paging
   write_cr0(read_cr0() | 0x80000000);
   paging_enabled = 1;
   Console::puts("\nPageTable::enable_paging enabled paging by setting bit 31 in CR0 register\n");
}

void PageTable::handle_fault(REGS * _r)
{
   Console::puts("\nPage Fault occured due to address - ");
   Console::puti(read_cr2());
   Console::puts("\n");

   unsigned int error_code = _r->err_code;
   unsigned long faulty_address = read_cr2();
   unsigned long kernel_rw_present_mask = 3, user_r_absent_mask = 4, user_rw_present_mask = 7;

   // get the first 10 bits to index the page table directory
   unsigned long pde_index = (faulty_address >> 22);

   // get the next 10 bits to index the page table page
   unsigned long pte_index = ((faulty_address >> 12) & 0x3FF);

   unsigned long * new_page_table_page;

   // if the last bit of error code is not set
   // page fault occured as the page is not present
   if ((error_code & 1) == 0) {

      // page table directory has an invalid entry (present bit is 0)
      if ((current_page_table->page_directory[pde_index] & 1) == 0) {
         // load a new page table page
         current_page_table->page_directory[pde_index] = kernel_mem_pool->get_frames(1) * PAGE_SIZE;
         current_page_table->page_directory[pde_index] |= kernel_rw_present_mask;

         // get the address of the page table page by clearing the last 12 bits
         // in the 32-bit page directory entry as the last 12 bits represent
         // various flags
         new_page_table_page = (unsigned long *) (current_page_table->page_directory[pde_index] & 0xFFFFF000);
         
         for (unsigned int index = 0; index < ENTRIES_PER_PAGE; index++) {
            // mark all entries as invalid
            // user bit is set to 1 as this page table page will
            // point to physical frames meant for user programs (above 4MB)
            new_page_table_page[index] = user_r_absent_mask;
          }

      } else {
         new_page_table_page = (unsigned long *) (current_page_table->page_directory[pde_index] & 0xFFFFF000);
         // load a new physical frame from process pool
         new_page_table_page[pte_index] = process_mem_pool->get_frames(1) * PAGE_SIZE;
         new_page_table_page[pte_index] |= user_rw_present_mask;
      }
   }

   Console::puts("Handled page fault\n");
}

void PageTable::free_page(unsigned long _page_no) {
    assert(false);
    Console::puts("freed page\n");
}