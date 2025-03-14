/*
 File: vm_pool.C
 
 Author:
 Date  : 2024/09/20
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {

    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    num_vm_regions = 0;

    // register the VM pool with the page table
    page_table->register_pool(this);

    struct vm_region * regions = (vm_region *) base_address;
    regions[0].base_address = base_address;
    regions[0].size = PageTable::PAGE_SIZE;

    vm_region_list = regions;

    // first VM region
    num_vm_regions = 1;

    Console::puts("VMPool Virtual Memory Pool Initialized!\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long num_pages = (_size / PageTable::PAGE_SIZE) + ((_size % PageTable::PAGE_SIZE) > 0 ? 1 : 0);
    
    // storing the newly allocated region in the VM region list
    vm_region_list[num_vm_regions].base_address = vm_region_list[num_vm_regions - 1].base_address +
    vm_region_list[num_vm_regions - 1].size;
    vm_region_list[num_vm_regions].size = num_pages * PageTable::PAGE_SIZE;

    num_vm_regions++;
    Console::puts("VMPool::allocate Allocated a new VM region from the VM pool\n");

    return vm_region_list[num_vm_regions - 1].base_address;
}

void VMPool::release(unsigned long _start_address) {
    unsigned int region_index = 0;

    while (region_index < num_vm_regions) {
        if (vm_region_list[region_index].base_address == _start_address) break;
        region_index++;
    }

    unsigned long num_pages = vm_region_list[region_index].size / PageTable::PAGE_SIZE;
    unsigned long start_address = _start_address;

    // free all the pages belonging to the VM region
    while (num_pages > 0) {
        page_table->free_page(start_address);
        start_address += PageTable::PAGE_SIZE;
        num_pages--;
    }

    // free the VM region
    // shift subsequent regions to the left by 1 place
    while (region_index < num_vm_regions - 1) {
        vm_region_list[region_index] = vm_region_list[region_index + 1];
        region_index++;
    }

    num_vm_regions--;

    Console::puts("VMPool::release Released memory region beginning at - ");
    Console::puti(_start_address);
    Console::puts("\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    // if issued address is out of bounds
    if (_address < base_address || _address > (base_address + size)) {
        Console::puts("VMPool::is_legitimate the issued address is not legitimate!\n");
        return false;
    }

    // issued address is valid
    Console::puts("VMPool::is_legitimate the issued address is legitimate!\n");
    return true;
}

