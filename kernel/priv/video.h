/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVVIDEO_H_
#define PRIVVIDEO_H_

#include "../pub/common.h"
#include "../pub/video.h"

#define COL_WOB			0x07				/* white on black */
#define VIDEO_BASE		0xC00B8000
#define COLS			80
#define ROWS			25
#define TAB_WIDTH		2

/**
 * Removes the BIOS-cursor from the screen
 */
static void vid_removeBIOSCursor(void);

#endif /* PRIVVIDEO_H_ */
