/**
 * @version		$Id: test.h 37 2008-11-09 16:58:20Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/mm.h"
#include "test.h"
#include "mm.h"

#define FRAME_COUNT 50

/* forward declarations */
static void test_mm(void);
static void test_printFreeFrames(void);
static void test_mm_allocate(memType type);
static void test_mm_free(memType type);

/* our test-module */
tTestModule tModMM = {
	"Physical memory-management",
	&test_mm
};

static u32 frames[FRAME_COUNT];

static void test_mm(void) {
	u32 freeDefFrames = mm_getNumberOfFreeFrames(MM_DEF);
	u32 freeDmaFrames = mm_getNumberOfFreeFrames(MM_DMA);

	test_caseStart("Requesting and freeing %d frames < 16MB...\n",FRAME_COUNT);
	test_mm_allocate(MM_DMA);
	test_mm_free(MM_DMA);
	if(freeDmaFrames != mm_getNumberOfFreeFrames(MM_DMA))
		test_caseFailed("oldDma=%d, newDma=%d",freeDmaFrames,mm_getNumberOfFreeFrames(MM_DMA));
	else
		test_caseSucceded();

	test_caseStart("Requesting and freeing %d frames > 16MB...\n",FRAME_COUNT);
	test_mm_allocate(MM_DEF);
	test_mm_free(MM_DEF);
	if(freeDefFrames != mm_getNumberOfFreeFrames(MM_DEF))
		test_caseFailed("oldDef=%d, newDef=%d",freeDefFrames,mm_getNumberOfFreeFrames(MM_DEF));
	else
		test_caseSucceded();
}

static void test_mm_allocate(memType type) {
	s32 i = 0;
	while(i < FRAME_COUNT) {
		frames[i] = mm_allocateFrame(type);
		i++;
	}
}

static void test_mm_free(memType type) {
	s32 i = FRAME_COUNT - 1;
	while(i >= 0) {
		mm_freeFrame(frames[i],type);
		i--;
	}
}
