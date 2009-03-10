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
	tFD self;
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

#define BUFFER_SIZE	256

/* the number of left-shifts for each state */
#define STATE_SHIFT	0
#define STATE_CTRL	1
#define STATE_ALT	2

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Sets the cursor
 */
static void vterm_setCursor(void);

/**
 * Prints the given character to screen
 *
 * @param c the character
 */
static void vterm_putchar(s8 c);

/**
 * Writes the given character to the video-driver (at the current position).
 * Will not move forward.
 *
 * @param c the character
 */
static void vterm_writeChar(u8 c);

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

void vterm_init(void) {
	s32 vidFd,selfFd;
	/* open video */
	do {
		vidFd = open("services:/video",IO_WRITE);
		if(vidFd < 0)
			yield();
	}
	while(vidFd < 0);

	/* open ourself to write keycodes into the receive-pipe (which can be read by other processes) */
	selfFd = open("services:/vterm",IO_WRITE);
	if(selfFd < 0) {
		printLastError();
		return;
	}

	/* init state */
	vterm.col = 0;
	vterm.row = ROWS - 1;
	vterm.foreground = WHITE;
	vterm.background = BLACK;
	vterm.shiftDown = false;
	vterm.altDown = false;
	vterm.ctrlDown = false;
	vterm.video = vidFd;
	vterm.self = selfFd;
}

void vterm_destroy(void) {
	close(vterm.video);
}

void vterm_handleKeycode(sMsgKbResponse *msg) {
	sKeymapEntry *e;
	s8 c;

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

	/* we don't need breakcodes anymore */
	if(msg->isBreak)
		return;

	e = keymap_get(msg->keycode);
	if(e != NULL) {
		if(vterm.shiftDown)
			c = e->shift;
		else if(vterm.altDown)
			c = e->alt;
		else
			c = e->def;

		if(c == NPRINT) {
			s8 escape[3] = {'\033',msg->keycode,(vterm.altDown << STATE_ALT) |
					(vterm.ctrlDown << STATE_CTRL) |
					(vterm.shiftDown << STATE_SHIFT)};
			write(vterm.self,&escape,sizeof(s8) * 3);
		}
		else
			write(vterm.self,&c,sizeof(s8));
	}
}

void vterm_puts(s8 *str) {
	s8 c;
	while((c = *str)) {
		if(c == '\e' || c == '\033') {
			str++;
			vterm_handleEscape(&str);
			continue;
		}
		vterm_putchar(c);
		str++;
	}
	vterm_setCursor();
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
	else if(c == '\b') {
		if(vterm.col > 0)
			vterm.col--;
		else if(vterm.row > 0) {
			vterm.row--;
			vterm.col = COLS -1;
		}
		/* overwrite */
		vterm_writeChar(' ');
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vterm_putchar(' ');
		}
	}
	else {
		vterm_writeChar(c);

		vterm.col++;
		/* do an explicit newline if necessary */
		if(vterm.col >= COLS)
			vterm_putchar('\n');
	}
}

static void vterm_writeChar(u8 c) {
	msgVidSet.data.character = c;
	msgVidSet.data.color = (vterm.background << 4) | vterm.foreground;
	msgVidSet.data.row = vterm.row;
	msgVidSet.data.col = vterm.col;
	write(vterm.video,&msgVidSet,sizeof(sMsgVidSet));
}

static void vterm_handleEscape(s8 **str) {
	u8 *fmt = *str;
	u8 keycode = *fmt;
	u8 value = *(fmt + 1);
	switch(keycode) {
		case VK_LEFT:
			if(vterm.col > 0) {
				vterm.col--;
				vterm_setCursor();
			}
			break;
		case VK_RIGHT:
			if(vterm.col < COLS - 1) {
				vterm.col++;
				vterm_setCursor();
			}
			break;
		case VK_HOME:
			if(value > 0) {
				if(value > vterm.col)
					vterm.col = 0;
				else
					vterm.col -= value;
				vterm_setCursor();
			}
			break;
		case VK_END:
			if(value > 0) {
				if(vterm.col + value > COLS - 1)
					vterm.col = COLS - 1;
				else
					vterm.col += value;
				vterm_setCursor();
			}
			break;
		case VK_ESC_RESET:
			vterm.foreground = WHITE;
			vterm.background = BLACK;
			break;
		case VK_ESC_FG:
			vterm.foreground = MIN(9,value);
			break;
		case VK_ESC_BG:
			vterm.background = MIN(9,value);
			break;
	}

	/* skip escape code */
	*str += 2;
}
