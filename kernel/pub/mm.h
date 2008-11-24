/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MM_H_
#define MM_H_

#include "../pub/common.h"
#include "../pub/multiboot.h"

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * M)

/* total mem size (in bytes) */
#define MEMSIZE					(mb->memUpper * K - KERNEL_P_ADDR)

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12
#define L16M_PAGE_COUNT			((16 * M - KERNEL_P_ADDR) / PAGE_SIZE)
#define U16M_PAGE_COUNT			((MEMSIZE / PAGE_SIZE) - L16M_PAGE_COUNT)

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((u32)(b) + (PAGE_SIZE - 1)) >> PAGE_SIZE_SHIFT)

#define L16M_CACHE_SIZE			16

/* set values to support bit-masks of the types */
typedef enum {MM_DMA = 1,MM_DEF = 2} eMemType;

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
void mm_allocateFrames(eMemType type,u32 *frames,u32 count);

/**
 * Allocates a frame of the given type and returns the frame-number
 *
 * @panic if there is no frame left anymore
 *
 * @param type the frame-type: MM_DMA or MM_DEF
 * @return the frame-number
 */
u32 mm_allocateFrame(eMemType type);

/**
 * A convenience-method to free multiple frames. Simply calls <count> times
 * mm_freeFrame(<frame>,<type>).
 *
 * @param type the frame-type: MM_DMA or MM_DEF
 * @param frames the array with <count> frames
 * @param count the number of frames you want to free
 */
void mm_freeFrames(eMemType type,u32 *frames,u32 count);

/**
 * Frees the given frame of the given type
 *
 * @param frame the frame-number
 * @param type the frame-type: MM_DMA or MM_DEF
 */
void mm_freeFrame(u32 frame,eMemType type);

/**
 * Prints all free frames
 */
void mm_printFreeFrames(void);

/**
 * Prints the bitmap for the lower 16MB memory
 */
void mm_printl16MBitmap(void);

#endif /*MM_H_*/
