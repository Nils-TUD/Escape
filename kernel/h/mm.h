/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MM_H_
#define MM_H_

#include "../h/common.h"
#include "../h/multiboot.h"

/*#define TEST_MM*/

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR	0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR		(1 * M)

/* total mem size (in bytes) */
#define MEMSIZE (mb->memUpper * K - KERNEL_P_ADDR)

#define PAGE_SIZE (4 * K)
#define PAGE_SIZE_SHIFT 12
#define L16M_PAGE_COUNT ((16 * M - KERNEL_P_ADDR) / PAGE_SIZE)
#define U16M_PAGE_COUNT ((MEMSIZE / PAGE_SIZE) - L16M_PAGE_COUNT)

#define L16M_CACHE_SIZE 16

/* set values to support bit-masks of the types */
typedef enum {MM_DMA = 1,MM_DEF = 2} memType;

/* start- and end-address of the kernel */
extern u32 KernelStart;
extern u32 KernelEnd;

/**
 * Initializes the memory-management
 */
void mm_init(void);

/**
 * Counts the number of free frames. This is primarly intended for debugging!
 * 
 * @param types a bit-mask with all types (MM_DMA,MM_DEF) to use for counting
 * @return the number of free frames
 */
u32 mm_getNumberOfFreeFrames(u32 types);

/**
 * A convenience-method to allocate multiple frames. Simply calls <count> times
 * mm_allocateFrame(<type>) and stores the frames in <frames>.
 * 
 * @param type the frame-type: MM_DMA or MM_DEF
 * @param frames the array for <count> frames
 * @param count the number of frames you want to get
 */
void mm_allocateFrames(memType type,u32* frames,u32 count);

/**
 * Allocates a frame of the given type and returns the frame-number
 * 
 * @panic if there is no frame left anymore
 * 
 * @param type the frame-type: MM_DMA or MM_DEF
 * @return the frame-number
 */
u32 mm_allocateFrame(memType type);

/**
 * A convenience-method to free multiple frames. Simply calls <count> times
 * mm_freeFrame(<frame>,<type>).
 * 
 * @param type the frame-type: MM_DMA or MM_DEF
 * @param frames the array with <count> frames
 * @param count the number of frames you want to free
 */
void mm_freeFrames(memType type,u32 *frames,u32 count);

/**
 * Frees the given frame of the given type
 * 
 * @param frame the frame-number
 * @param type the frame-type: MM_DMA or MM_DEF
 */
void mm_freeFrame(u32 frame,memType type);

/**
 * Prints the bitmap for the lower 16MB memory
 */
void mm_printl16MBitmap(void);

#ifdef TEST_MM
void test_mm(void);
#endif

#endif /*MM_H_*/
