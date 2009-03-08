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
#define TAB_WIDTH	2

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/* our state */
typedef struct {
	u8 *data;
	u8 col;
	u8 row;
	u8 foreground;
	u8 background;
} sVideo;

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void);
/**
 * Prints the given character at the current position
 *
 * @param c the character
 */
static void vid_putchar(s8 c);
/**
 * Prints the given string at the current position
 *
 * @param str the string
 */
static void vid_puts(s8 *str);

static sVideo video;

s32 main(void) {
	s32 id = regService("video",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* map video-memory for our process */
	video.data = (u8*)mapPhysical(VIDEO_MEM,COLS * (ROWS + 2) * 2 + 1);
	if(video.data == 0) {
		printLastError();
		return 1;
	}

	/* init state */
	video.col = 0;
	video.row = 0;
	video.foreground = WHITE;
	video.background = BLACK;

	/* request io-ports for qemu and bochs */
	requestIOPort(0xe9);
	requestIOPort(0x3f8);
	requestIOPort(0x3fd);

	/* clear screen */
	memset(video.data,0,COLS * ROWS * 2);

	/* wait for messages */
	static sMsgVidRequest msg;
	while(1) {
		s32 fd = waitForClient(id);
		if(fd < 0)
			printLastError();
		else {
			/* read all available messages */
			s32 c;
			do {
				/* read header */
				if((c = read(fd,&msg,sizeof(sMsgVidRequest))) < 0)
					printLastError();
				else if(c > 0) {
					/* see what we have to do */
					switch(msg.id) {
						/* print character */
						case MSG_VIDEO_PUTCHAR: {
							s8 character;
							read(fd,&character,sizeof(s8));
							vid_putchar(character);
						}
						break;

						/* print string */
						case MSG_VIDEO_PUTS: {
							s8 *readBuf = malloc(msg.length * sizeof(s8));
							read(fd,readBuf,msg.length);
							/* ensure termination */
							*(readBuf + msg.length - 1) = '\0';
							/*vid_puts((s8*)"Read: ");*/
							vid_puts(readBuf);
							/*vid_putchar('\n');*/
							free(readBuf);
						}
						break;

						/* goto x/y */
						case MSG_VIDEO_GOTO: {
							static sMsgDataVidGoto msgData;
							read(fd,&msgData,sizeof(msgData));
							debugf("col=%d, row=%d\n",msgData.col,msgData.row);
							if(msgData.col < COLS)
								video.col = msgData.col;
							if(msgData.row < ROWS)
								video.row = msgData.row;
						}
						break;
					}
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);

	unregService(id);
	return 0;
}


static void vid_move(void) {
	u32 i;
	s8 *src,*dst;
	/* last line? */
	if(video.row >= ROWS) {
		/* copy all chars one line back */
		src = (s8*)(video.data + COLS * 2);
		dst = (s8*)video.data;
		for(i = 0; i < ROWS * COLS * 2; i++) {
			*dst++ = *src++;
		}
		/* to prev line */
		video.col = 0;
		video.row--;
	}
}

static void vid_putchar(s8 c) {
	u32 i;
	vid_move();

	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r') {
		outb(0xe9,c);
	    outb(0x3f8,c);
	    while((inb(0x3fd) & 0x20) == 0);
	}

	if(c == '\n') {
		/* to next line */
		video.row++;
		/* move cursor to line start */
		vid_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		video.col = 0;
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vid_putchar(' ');
		}
	}
	else {
		s8 *data = video.data + (video.row * COLS * 2) + (video.col * 2);
		*data = c;
		data++;
		*data = (video.background << 4) | video.foreground;

		video.col++;
		/* do an explicit newline if necessary */
		if(video.col >= COLS)
			vid_putchar('\n');
	}
}

static void vid_puts(s8 *str) {
	while(*str) {
		vid_putchar(*str++);
	}
}
