/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    inodes = new Inode[MAX_INODES];
    free_blocks = new unsigned char[SimpleDisk::BLOCK_SIZE]; 
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    
    disk->write(INODE_BLOCK_NO, (unsigned char *) inodes);
    disk->write(FREELIST_BLOCK_NO, free_blocks);

	delete []inodes;
	delete []free_blocks;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

int FileSystem::GetFreeInode() {
    unsigned short index = 0;
    while (index < MAX_INODES) {
        // return index of free inode
        if (inodes[index].id == END) return index;
        index++;
    }
    // return -1 if there are no free inodes
    return END;
}

int FileSystem::GetFreeBlock() {
    unsigned int index = 0;
    while (index < SimpleDisk::BLOCK_SIZE) {
        // return index of free block
        if (free_blocks[index] == 0) return index;
        index++;
    }
    // return -1 if there are no free blocks
    return END;
}

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");
    /* Here you read the inode list and the free list into memory */
    
    disk = _disk;

    // read the inode list from block 0
    disk->read(INODE_BLOCK_NO, (unsigned char *) inodes);

    // read the free list from block 1
    disk->read(FREELIST_BLOCK_NO, free_blocks);

    // ensure inode block and free list block have valid data
    if (free_blocks[0] == 1 && free_blocks[1] == 1) {
        return true;
    } else {
        return false;
    }
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    
    unsigned int index = 0;
    unsigned char buffer[SimpleDisk::BLOCK_SIZE];

    // fill the buffer with 0xff
    while (index < SimpleDisk::BLOCK_SIZE) {
        buffer[index++] = 0xff;
    }
    _disk->write(INODE_BLOCK_NO, buffer); // flush the inode block

    index = 0;

    // fill the buffer with 0x00
    while (index < SimpleDisk::BLOCK_SIZE) {
        buffer[index++] = 0x00;
    }

    // marking first 2 blocks as occupied
    buffer[INODE_BLOCK_NO] = 1;
    buffer[FREELIST_BLOCK_NO] = 1;

    _disk->write(FREELIST_BLOCK_NO, buffer); // flush the free list block except inode and free list blocks
    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
   
    unsigned int index = 0;
    while (index < MAX_INODES) {
        if (inodes[index].id == _file_id) return &inodes[index];
        index++;
    }

    Console::puts("FileSystem::LookupFile File with ID - ");
    Console::puti(_file_id);
    Console::puts(" does not exist!\n");
    return nullptr;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
   
    if (LookupFile(_file_id) != nullptr) {
        Console::puts("FileSystem::CreateFile File with ID - ");
        Console::puti(_file_id);
        Console::puts(" already exists!\n");
        return false;
    }

    int free_block_no = GetFreeBlock();
    if (free_block_no == END) {
        Console::puts("FileSystem::CreateFile Out of free blocks!\n");
        return false;
    }

    int free_inode_no = GetFreeInode();
    if (free_inode_no == END) {
        Console::puts("FileSystem::CreateFile Out of free inodes!\n");
        return false;
    }

    // mark the free block as allocated
    free_blocks[free_block_no] = 1;

    // store metadata in inode
    inodes[free_inode_no].id = _file_id;
    inodes[free_inode_no].size = 0;
    inodes[free_inode_no].block_no = free_block_no;
    inodes[free_block_no].fs = this;

    // write the inode and free list blocks on disk
    disk->write(INODE_BLOCK_NO, (unsigned char *) inodes);
    disk->write(FREELIST_BLOCK_NO, free_blocks);

    Console::puts("FileSystem::CreateFile created a new file with ID - ");
    Console::puti(_file_id);
    Console::puts("\n");
    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */
    
    Inode * inode = LookupFile(_file_id);
    if (inode == nullptr) {
        Console::puts("FileSystem::DeleteFile File with ID - ");
        Console::puti(_file_id);
        Console::puts(" does not exist!\n");
        return false;
    }

    free_blocks[inode->block_no] = 0;
    inode->id = END;
    inode->block_no = END;
    inode->size = END;

    // update the inode and free list blocks
    disk->write(INODE_BLOCK_NO, (unsigned char *) inodes);
    disk->write(FREELIST_BLOCK_NO, free_blocks);

    Console::puts("FileSystem::DeleteFile deleted file with ID - ");
    Console::puti(_file_id);
    Console::puts("\n");
    return true;
}

// writes the inode block to disk
void FileSystem::write_inode_block_to_disk() {
    disk->write(INODE_BLOCK_NO, (unsigned char *) inodes);
}

// writes the buffer content to the specified block on disk
void FileSystem::write_block_to_disk(unsigned long block_number, unsigned char * buffer) {
    disk->write(block_number, buffer);
}

// reads the from the specified block number and writes into the buffer
void FileSystem::read_block_from_disk(unsigned long block_number, unsigned char * buffer) {
    disk->read(block_number, buffer);
}