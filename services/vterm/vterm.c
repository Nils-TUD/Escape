/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <io.h>
#include <proc.h>
#include <keycodes.h>
#include <heap.h>
#include <string.h>
#include "vterm.h"
#include "keymap.h"

/* our vterm-state */
typedef struct {
	u8 col;
	u8 row;
	u8 foreground;
	u8 background;
	bool shiftDown;
	bool altDown;
	bool ctrlDown;
	tFD video;
} sVTerm;

/* the messages we'll send */
typedef struct {
	sMsgDefHeader header;
	sMsgDataVidSet data;
} __attribute__((packed)) sMsgVidSet;
typedef struct {
	sMsgDefHeader header;
	sMsgDataVidSetCursor data;
} __attribute__((packed)) sMsgVidSetCursor;

#define COLS		80
#define ROWS		25
#define TAB_WIDTH	2

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Sets the cursor
 */
static void vterm_setCursor(void);

/**
 * Deletes the next character while reading a line
 */
static void vterm_deleteNext(void);

/**
 * Deletes the last character while reading a line
 */
static void vterm_deletePrev(void);

/**
 * Prints the given character to screen
 *
 * @param c the character
 */
static void vterm_putchar(s8 c);

/**
 * Handles the escape-code
 *
 * @param str the current position (will be changed)
 */
static void vterm_handleEscape(s8 **str);

/* escape-code-color to video-color */
static u8 colorTable[] = {
	BLACK,
	RED,
	GREEN,
	ORANGE /* should be brown */,
	BLUE,
	MARGENTA,
	CYAN,
	GRAY
};

/* the video-set message */
static sMsgVidSet msgVidSet = {
	.header = {
		.id = MSG_VIDEO_SET,
		.length = sizeof(sMsgDataVidSet)
	},
	.data = {
		.col = 0,
		.row = 0,
		.color = 0,
		.character = 0
	}
};
/* the set-cursor message */
static sMsgVidSetCursor msgVidSetCursor = {
	.header = {
		.id = MSG_VIDEO_SETCURSOR,
		.length = sizeof(sMsgDataVidSetCursor)
	},
	.data = {
		.col = 0,
		.row = 0
	}
};
/* the move-up message */
static sMsgDefHeader msgVidMove = {
	.id = MSG_VIDEO_MOVEUP,
	.length = 0
};

static sVTerm vterm;
static u8 readStart,readEnd;
static bool readingFinished = false;
static u16 bufferSize = 0;
static s8 *buffer = NULL;

void vterm_init(void) {
	s32 vidFd;
	/* open video */
	do {
		vidFd = open("services:/video",IO_WRITE);
		if(vidFd < 0)
			yield();
	}
	while(vidFd < 0);

	/* init state */
	vterm.col = 0;
	vterm.row = ROWS - 1;
	vterm.foreground = WHITE;
	vterm.background = BLACK;
	vterm.shiftDown = false;
	vterm.altDown = false;
	vterm.ctrlDown = false;
	vterm.video = vidFd;
}

void vterm_destroy(void) {
	close(vterm.video);
}

bool vterm_isReading(void) {
	return buffer != NULL && !readingFinished;
}

void vterm_startReading(u16 maxLength) {
	/* free previously allocated mem */
	if(buffer)
		free(buffer);

	buffer = (s8*)malloc(sizeof(s8) * maxLength);
	/* ignore error here, since buffer is still NULL */
	readingFinished = false;
	readStart = vterm.col;
	readEnd = vterm.col;
	bufferSize = maxLength;
}

u16 vterm_getReadLength(void) {
	if(buffer == NULL || !readingFinished)
		return 0;
	return bufferSize;
}

s8 *vterm_getReadLine(void) {
	if(buffer == NULL || !readingFinished)
		return NULL;
	return buffer;
}

void vterm_handleKeycode(sMsgKbResponse *msg) {
	/* in readline mode? */
	if(buffer != NULL) {
		/* handle shift, alt and ctrl */
		switch(msg->keycode) {
			case VK_LSHIFT:
			case VK_RSHIFT:
				vterm.shiftDown = !msg->isBreak;
				break;
			case VK_LALT:
			case VK_RALT:
				vterm.altDown = !msg->isBreak;
				break;
			case VK_LCTRL:
			case VK_RCTRL:
				vterm.ctrlDown = !msg->isBreak;
				break;
		}

		if(!msg->isBreak) {
			sKeymapEntry *e;
			s8 c;
			switch(msg->keycode) {
				case VK_LEFT:
					if(vterm.col > readStart)
						vterm.col--;
					vterm_setCursor();
					break;
				case VK_RIGHT:
					if(vterm.col < readEnd)
						vterm.col++;
					vterm_setCursor();
					break;
				case VK_HOME:
					if(vterm.col != readStart) {
						vterm.col = readStart;
						vterm_setCursor();
					}
					break;
				case VK_END:
					if(vterm.col != readEnd) {
						vterm.col = readEnd;
						vterm_setCursor();
					}
					break;
				case VK_DELETE:
					vterm_deleteNext();
					break;
				case VK_BACKSP:
					vterm_deletePrev();
					break;
			}

			e = keymap_get(msg->keycode);
			if(e != NULL) {
				if(vterm.shiftDown)
					c = e->shift;
				else if(vterm.altDown)
					c = e->alt;
				else
					c = e->def;

				if(c != NPRINT) {
					u16 pos = vterm.col - readStart;
					if(c == '\n' || pos >= bufferSize) {
						readingFinished = true;
						buffer[bufferSize - 1] = '\0';
						vterm_putchar('\n');
					}
					else {
						buffer[pos] = c;
						if(vterm.col == readEnd)
							readEnd++;
						vterm_putchar(c);
					}
					vterm_setCursor();
				}
				/*yield();*/
			}
		}
	}
}

void vterm_puts(s8 *str) {
	s8 c;
	while((c = *str)) {
		if(c == '\e' || c == '\033') {
			if(*(str + 1) == '[') {
				str += 2;
				vterm_handleEscape(&str);
				continue;
			}
		}
		vterm_putchar(c);
		str++;
	}
	vterm_setCursor();
}

static void vterm_deleteNext(void) {
	if(vterm.col < readEnd) {
		/* to next char */
		vterm.col++;
		/* now we can delete the prev char */
		vterm_deletePrev();
	}
}

static void vterm_deletePrev(void) {
	u16 col = vterm.col;
	if(col > readStart) {
		/* are we at the end? */
		if(col == readEnd) {
			vterm.col--;
			vterm_putchar(' ');
			vterm.col--;
		}
		else {
			u16 pos = col - readStart;
			/* move chars in the buffer */
			memmove(buffer + pos - 1,buffer + pos,readEnd - col);
			/* refresh screen */
			vterm.col--;
			col--;
			while(col < readEnd - 1) {
				vterm_putchar(buffer[col - readStart]);
				col++;
			}
			/* delete last char */
			vterm_putchar(' ');
			/* set column (putchar changed it) */
			vterm.col = pos + readStart - 1;
		}
		/* refresh cursor-pos */
		vterm_setCursor();
		readEnd--;
	}
}

static void vterm_setCursor(void) {
	msgVidSetCursor.data.col = vterm.col;
	msgVidSetCursor.data.row = vterm.row;
	write(vterm.video,&msgVidSetCursor,sizeof(sMsgVidSetCursor));
}

static void vterm_putchar(s8 c) {
	u32 i;

	/* move all one line up, if necessary */
	if(vterm.row >= ROWS) {
		write(vterm.video,&msgVidMove,sizeof(sMsgDefHeader));
		vterm.row--;
	}

	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r') {
		outb(0xe9,c);
		outb(0x3f8,c);
		while((inb(0x3fd) & 0x20) == 0);
	}

	if(c == '\n') {
		/* to next line */
		vterm.row++;
		/* move cursor to line start */
		vterm_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		vterm.col = 0;
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vterm_putchar(' ');
		}
	}
	else {
		msgVidSet.data.character = c;
		msgVidSet.data.color = (vterm.background << 4) | vterm.foreground;
		msgVidSet.data.row = vterm.row;
		msgVidSet.data.col = vterm.col;
		write(vterm.video,&msgVidSet,sizeof(sMsgVidSet));

		vterm.col++;
		/* do an explicit newline if necessary */
		if(vterm.col >= COLS)
			vterm_putchar('\n');
	}
}

static void vterm_handleEscape(s8 **str) {
	cstring fmt = *str;
	while(1) {
		/* read code */
		u8 colCode = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			colCode = colCode * 10 + (*fmt - '0');
			fmt++;
		}

		switch(colCode) {
			/* reset all */
			case 0:
				vterm.foreground = WHITE;
				vterm.background = BLACK;
				break;

			/* foreground */
			case 30 ... 37:
				vterm.foreground = colorTable[colCode - 30];
				break;

			/* default foreground */
			case 39:
				vterm.foreground = WHITE;
				break;

			/* background */
			case 40 ... 47:
				vterm.background = colorTable[colCode - 40];
				break;

			/* default background */
			case 49:
				vterm.background = BLACK;
				break;
		}

		/* end of code? */
		if(*fmt == 'm') {
			fmt++;
			break;
		}
		/* we should stop on \0, too */
		if(*fmt == '\0')
			break;
		/* otherwise skip ';' */
		fmt++;
	}

	*str = fmt;
}
