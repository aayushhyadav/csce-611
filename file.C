/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    fs_ptr = _fs;

    // fetch the corresponding inode
    inode_ptr = fs_ptr->LookupFile(_id);
    fd_position = 0;

    // read file contents and store in the cache
    fs_ptr->read_block_from_disk(inode_ptr->block_no, block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    
    // update the disk block before closing the file
    fs_ptr->write_block_to_disk(inode_ptr->block_no, block_cache);

    // update the inode
    fs_ptr->write_inode_block_to_disk();
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    
    unsigned int bytes_read = 0;
    while (bytes_read < _n && !EoF()) {
        // copy contents from in-memory cache to the input buffer
        _buf[bytes_read++] = block_cache[fd_position++];
    }
    return bytes_read;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    
    unsigned int bytes_written = 0;

    if (inode_ptr->size < fd_position + _n) {
        inode_ptr->size = fd_position + _n;
    }
    if (inode_ptr->size > SimpleDisk::BLOCK_SIZE) {
        inode_ptr->size = SimpleDisk::BLOCK_SIZE;
    }

    while (!EoF()) {
        // copy contents of the buffer into the in-memory cache
        block_cache[fd_position++] = _buf[bytes_written++];
    }
    return bytes_written;
}

void File::Reset() {
    Console::puts("resetting file\n");
    fd_position = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    return (fd_position == inode_ptr->size) ? true : false;
}
