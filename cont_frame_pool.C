/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/
ContFramePool* ContFramePool::head = nullptr;
ContFramePool* ContFramePool::tail = nullptr;
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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
	unsigned int bitmap_index = _frame_no / 4;
  unsigned char mask = 0x1 << ((_frame_no % 4) * 2);

	unsigned char first_bit = bitmap[bitmap_index] & mask;
	unsigned char second_bit = bitmap[bitmap_index] & (mask << 1);

	if (first_bit > 0 && second_bit > 0) return FrameState::Free;
	else if (first_bit == 0 && second_bit > 0) return FrameState::HoS;
	else return FrameState::Used;
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
  unsigned int bitmap_index = _frame_no / 4;
  unsigned char mask = 0x1 << ((_frame_no % 4) * 2);

  switch(_state) {
		// Used state is represented by 00
    case FrameState::Used:
    bitmap[bitmap_index] ^= mask;
    bitmap[bitmap_index] ^= (mask << 1);
    break;

		// Free state is represented by 11
		case FrameState::Free:
			bitmap[bitmap_index] |= mask;
			bitmap[bitmap_index] |= (mask << 1);
			break;

		// Head of Sequence state is represented by 10
		case FrameState::HoS:
			bitmap[bitmap_index] ^= mask;
  }  
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // Bitmap must fit in a single frame!
    assert(_n_frames <= FRAME_SIZE * 4);
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    // frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Everything ok. Proceed to mark all frame as free.
    for(int fno = 0; fno < nframes; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    // Mark the first frame as being head of sequence if it is being used
    if(_info_frame_no == 0) {
        set_state(0, FrameState::HoS);
        nFreeFrames--;
    }
    
	// frame pool management
	if (head == nullptr) {
		head = this;
		head->next = nullptr;
		tail = head;

	} else {
		tail->next = this;
		tail = tail->next;
		tail->next = nullptr;
	}

	// based on starting frame, set the pool type
	if (base_frame_no == KERNEL_POOL_START_FRAME) tail->type = 0;
	else tail->type = 1;

    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
	// Enough frames to allocate?
    assert(nFreeFrames >= _n_frames);

	unsigned int start_frame_number = 0, frame_sequence_length = 0, fn = 0;

	while (fn < nframes) {
		if (get_state(fn) == FrameState::Free) {
			start_frame_number = fn;
			frame_sequence_length = 1;
			fn++;

			// look for a contiguous sequence of frames with the requested length
			while (get_state(fn) == FrameState::Free) {
				if (frame_sequence_length == _n_frames) break;
				frame_sequence_length++;
				fn++;
			}

			if (frame_sequence_length == _n_frames) break;
		}
		fn++;
	}

	// we can allocate memory
	if (frame_sequence_length == _n_frames) {
		mark_inaccessible(base_frame_no + start_frame_number, _n_frames);
		Console::puts("ContFramePool::get_frames successfully allocated the required frames!\n");
		return (unsigned long) start_frame_number + base_frame_no;
	}

	Console::puts("ContFramePool::get_frames detected external fragmentation. Cannot allocate the requested frames!\n");
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
	unsigned long start_frame_number = _base_frame_no - base_frame_no;
	unsigned long fno;

	if (get_state(start_frame_number) != FrameState::Free) {
		Console::puts("ContFramePool::mark_inaccessible cannot perform operation on an already allocated frame!\n");
		return;
	}
	
	// mark the first frame as head of sequence
	set_state(start_frame_number, FrameState::HoS);

	for (fno = start_frame_number + 1; fno < start_frame_number + _n_frames; fno++) {
		// mark the remaining frames as used
		set_state(fno, FrameState::Used);
	}

	nFreeFrames -= _n_frames;
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
		unsigned int pool_type;

		// determine which pool the frame belongs to
		if (_first_frame_no >= KERNEL_POOL_START_FRAME &&
		_first_frame_no < (KERNEL_POOL_START_FRAME + KERNEL_POOL_SIZE)) {
			pool_type = 0;

		} else {
			pool_type = 1;
		}

		ContFramePool* cur_node = head;
		
		// invoke the designated pool's release_frame function
		while (cur_node != nullptr) {
			if (cur_node->type == pool_type) {
				cur_node->pool_release_frame(_first_frame_no);
				return;
			}
			cur_node = cur_node->next;
		}
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
	unsigned long round_off =  (_n_frames % NUMBER_OF_FRAMES_MANAGED_FROM_ONE_FRAME) > 0 ? 1 : 0; 
	return (_n_frames / NUMBER_OF_FRAMES_MANAGED_FROM_ONE_FRAME) + round_off;
}

void ContFramePool::pool_release_frame(unsigned long _first_frame_no)
{
		unsigned long fno = _first_frame_no - base_frame_no;

		if (get_state(fno) == FrameState::HoS) {
			set_state(fno, FrameState::Free);
			nFreeFrames++;

		} else {
			Console::puts("ContFramePool::pool_release_frame first frame not marked as HoS! Cannot free the requested frames!\n");
		}

		fno++;

		while (get_state(fno) == FrameState::Used) {
			set_state(fno, FrameState::Free);
			nFreeFrames++;
			fno++;
		}

		Console::puts("ContFramePool::pool_release_frame successfully freed the allocated frames!\n");
}