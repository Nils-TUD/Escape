/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <service.h>
#include <io.h>
#include <messages.h>
#include <heap.h>
#include <mem.h>
#include <string.h>
#include <debug.h>

/* the physical memory of the 80x25 device */
#define VIDEO_MEM	0xB8000

#define COLS		80
#define ROWS		25

/* our state */
static u8 *videoData;

/**
 * Moves all lines one line up
 */
static void vid_moveUp(void);

s32 main(void) {
	s32 id;

	debugf("Service video has pid %d\n",getpid());

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

	/* clear screen */
	memset(videoData,0,COLS * ROWS * 2);

	/* wait for messages */
	static sMsgDefHeader msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0)
			yield();
		else {
			/* read all available messages */
			s32 c;
			do {
				/* read header */
				if((c = read(fd,&msg,sizeof(sMsgDefHeader))) < 0)
					printLastError();
				else if(c > 0) {
					/* see what we have to do */
					switch(msg.id) {
						/* set character */
						case MSG_VIDEO_SET: {
							static sMsgDataVidSet data;
							if(read(fd,&data,sizeof(sMsgDataVidSet)) > 0) {
								/*debugf("Got %d, color %d for row %d, col %d\n",data.character,
									data.color,data.row,data.col);*/
								u8 *ptr = videoData + (data.row * COLS * 2) + data.col * 2;
								*ptr = data.character;
								*(ptr + 1) = data.color;
							}
						}
						break;

						/* move up */
						case MSG_VIDEO_MOVEUP: {
							vid_moveUp();
						}
						break;
					}
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	unregService(id);
	return 0;
}


static void vid_moveUp(void) {
	u32 i;
	s8 *src,*dst;
	/* copy all chars one line back */
	src = (s8*)(videoData + COLS * 2);
	dst = (s8*)videoData;
	for(i = 0; i < ROWS * COLS * 2; i++) {
		*dst++ = *src++;
	}
}
