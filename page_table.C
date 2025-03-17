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
VMPool * PageTable::vm_pool_head = nullptr;
VMPool * PageTable::vm_pool_tail = nullptr;


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
   unsigned long * page_table = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);

   unsigned long address = 0, kernel_rw_present_mask = 3, kernel_rw_absent_mask = 2;
   unsigned int pte, pde;

   // direct map the first 4MB of memory
   for (pte = 0; pte < ENTRIES_PER_PAGE; pte++) {
      page_table[pte] = address | kernel_rw_present_mask;
      address += PAGE_SIZE;
   }

   // make the last entry of page directory point to itself
   page_directory[1023] = ((unsigned long) page_directory | kernel_rw_present_mask);

   // populate the first entry in the page table directory
   page_directory[0] = (unsigned long) page_table;
   page_directory[0] |= kernel_rw_present_mask;

   // populate the remaining page directory entries
   for (pde = 1; pde < ENTRIES_PER_PAGE - 1; pde++) {
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
   unsigned long * new_physical_frame;
   unsigned long * pde_addr;

   // if the last bit of error code is not set
   // page fault occured as the page is not present
   if ((error_code & 1) == 0) {

      unsigned int present_flag = 0;
      VMPool * cur_vm_pool = vm_pool_head;

      // verify if the faulty address is valid
      // iterate over VM pool regions
      while (cur_vm_pool != nullptr) {
         if (cur_vm_pool->is_legitimate(faulty_address)) {
            present_flag = 1;
            break;
         }
         cur_vm_pool = cur_vm_pool->next_pool;
      }

      if (cur_vm_pool != nullptr && present_flag == 0) {
         Console::puts("PageTable::handle_fault the faulty address is not legitimate!\n");
         assert(false);
      }

      // page table directory has an invalid entry (present bit is 0)
      if ((current_page_table->page_directory[pde_index] & 1) == 0) {
         // load a new page table page
         new_page_table_page = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);

         // access the PDE
         pde_addr = PDE_address();
         pde_addr[pde_index] = ((unsigned long) new_page_table_page | kernel_rw_present_mask);

         for (unsigned int index = 0; index < ENTRIES_PER_PAGE; index++) {
            // mark all entries as invalid
            // user bit is set to 1 as this page table page will
            // point to physical frames meant for user programs (above 4MB)
            new_page_table_page[index] = user_r_absent_mask;
          }

      } else {
         new_physical_frame = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);

         // generate the page table page address
         unsigned long * page_table_page = PTE_address(faulty_address);

         page_table_page[pte_index] = ((unsigned long) new_physical_frame | user_rw_present_mask);
      }
   }

   Console::puts("Handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    // head points to the first VM pool
    if (vm_pool_head == nullptr) {
        vm_pool_head = _vm_pool;
        vm_pool_tail = _vm_pool;
        vm_pool_tail->next_pool = nullptr;

    // subsequent VM pools are added at the tail
    } else {
        vm_pool_tail->next_pool = _vm_pool;
        vm_pool_tail = vm_pool_tail->next_pool;
        vm_pool_tail->next_pool = nullptr;
    }
}

void PageTable::free_page(unsigned long _page_no) {
   // get the first 10 bits to index the page table directory
   unsigned long pde_index = (_page_no >> 22);

   // get the next 10 bits to index the page table page
   unsigned long pte_index = ((_page_no >> 12) & 0x3FF);

   // generate the page table page address
   unsigned long * page_table_page = PTE_address(_page_no);

   // compute the frame number
   // first 20 bits of the page table page entry gives
   // the first 20 bits of the physical address
   // last 12 bits contain flags and are hence, cleared
   unsigned long frame_num = (page_table_page[pte_index] & 0xFFFFF000) / PageTable::PAGE_SIZE;

   // free the physical frame
   process_mem_pool->release_frames(frame_num);

   // mark the page table page entry as invalid
   page_table_page[pte_index] &= 0xFFFFFFFE;

   // flush the TLB
   load();

   Console::puts("PageTable::free_page page freed!\n");
}

unsigned long * PageTable::PDE_address() {
   // this is interpreted as 1023 | 1023
   return (unsigned long *) (0xFFFFF000);
}

unsigned long * PageTable::PTE_address(unsigned long addr) {
   // get the first 10 bits to index the page table directory
   unsigned long pde_index = (addr >> 22);
   // this is interpreted as 1023 | PDE
   return (unsigned long *) ((0x000003FF << 22) | (pde_index << 12));
}