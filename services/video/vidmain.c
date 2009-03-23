/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <service.h>
#include <io.h>
#include <ports.h>
#include <messages.h>
#include <heap.h>
#include <mem.h>
#include <string.h>
#include <debug.h>
#include <proc.h>

/* the physical memory of the 80x25 device */
#define VIDEO_MEM			0xB8000

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX	0x3D4
#define CURSOR_PORT_DATA	0x3D5
#define CURSOR_DATA_LOCLOW	0x0F
#define CURSOR_DATA_LOCHIGH	0x0E

#define COLS		80
#define ROWS		25

/**
 * Sets the cursor to given position
 *
 * @param row the row
 * @param col the col
 */
static void vid_setCursor(u8 row,u8 col);

/* our state */
static u8 *videoData;

/**
 * Sets the screen-content
 *
 * @param startPos the position where to start (in chars [character+color], not bytes)
 * @param buffer the buffer
 * @param length the buffer-size
 */
static void vid_setScreen(u16 startPos,s8 *buffer,u32 length);

s32 main(void) {
	s32 id;

	id = regService("video",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* map video-memory for our process */
	videoData = (u8*)mapPhysical(VIDEO_MEM,COLS * (ROWS + 1) * 2);
	if(videoData == NULL) {
		printLastError();
		return 1;
	}

	/* reserve ports for cursor */
	requestIOPort(CURSOR_PORT_INDEX);
	requestIOPort(CURSOR_PORT_DATA);

	/* clear screen */
	memset(videoData,0,COLS * ROWS * 2);

	/* wait for messages */
	static sMsgHeader msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0)
			sleep(EV_CLIENT);
		else {
			/* read all available messages */
			while(read(fd,&msg,sizeof(sMsgHeader)) > 0) {
				/* see what we have to do */
				switch(msg.id) {
					/* set character */
					case MSG_VIDEO_SET: {
						static sMsgDataVidSet data;
						if(read(fd,&data,sizeof(sMsgDataVidSet)) > 0) {
							if(data.row < ROWS && data.col < COLS) {
								u8 *ptr = videoData + (data.row * COLS * 2) + data.col * 2;
								*ptr = data.character;
								*(ptr + 1) = data.color;
								vid_setCursor(data.row,data.col + 1);
							}
						}
					}
					break;

					/* set screen */
					case MSG_VIDEO_SETSCREEN: {
						if(msg.length > sizeof(u16) && msg.length <= COLS * ROWS * 2 + sizeof(u16)) {
							u16 *startPos;
							s8 *buf = (s8*)malloc(msg.length * sizeof(s8));
							read(fd,buf,msg.length);
							startPos = (u16*)buf;
							if(*startPos < COLS * ROWS - 1) {
								if(msg.length - sizeof(u16) <= (u32)*startPos * 2 + COLS * ROWS * 2)
									vid_setScreen(*startPos,buf + sizeof(u16),msg.length - sizeof(u16));
							}
							free(buf);
						}
					}
					break;

					/* set cursor */
					case MSG_VIDEO_SETCURSOR: {
						static sMsgDataVidSetCursor data;
						if(read(fd,&data,sizeof(sMsgDataVidSetCursor)) > 0) {
							if(data.row < ROWS && data.col < COLS) {
								vid_setCursor(data.row,data.col);
							}
						}
					}
					break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(CURSOR_PORT_INDEX);
	releaseIOPort(CURSOR_PORT_DATA);
	unregService(id);
	return 0;
}

static void vid_setCursor(u8 row,u8 col) {
   u16 position = (row * COLS) + col;

   /* cursor LOW port to vga INDEX register */
   outByte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
   outByte(CURSOR_PORT_DATA,(u8)(position & 0xFF));
   /* cursor HIGH port to vga INDEX register */
   outByte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
   outByte(CURSOR_PORT_DATA,(u8)((position >> 8) & 0xFF));
}

static void vid_setScreen(u16 startPos,s8 *buffer,u32 length) {
	s8 *ptr = (s8*)videoData + startPos * 2;
	while(length-- > 0)
		*ptr++ = *buffer++;
}
