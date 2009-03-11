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

#define COLS				80
#define ROWS				25
#define TAB_WIDTH			2

/* the number of lines to keep in history */
#define HISTORY_SIZE		(ROWS * 2)
#define BUFFER_SIZE			(COLS * 2 * HISTORY_SIZE)

/* the number of left-shifts for each state */
#define STATE_SHIFT			0
#define STATE_CTRL			1
#define STATE_ALT			2

/* our vterm-state */
typedef struct {
	/* position (on the current page) */
	u8 col;
	u8 row;
	/* colors */
	u8 foreground;
	u8 background;
	/* key states */
	bool shiftDown;
	bool altDown;
	bool ctrlDown;
	/* file-descriptors */
	tFD video;
	tFD self;
	/* the first line with content */
	u16 firstLine;
	/* the line where row+col starts */
	u16 currLine;
	/* the first visible line */
	u16 firstVisLine;
	/* in message form for performance-issues */
	struct {
		sMsgDefHeader header;
		s8 data[BUFFER_SIZE];
	} __attribute__((packed)) buffer;
	struct {
		sMsgDefHeader header;
		s8 data[COLS * 2];
	} __attribute__((packed)) titleBar;
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
 * Inserts a new line
 */
static void vterm_newLine(void);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param lines the number of lines to move
 */
static void vterm_scroll(s16 lines);

/**
 * Refreshes the screen
 */
static void vterm_refreshScreen(void);

/**
 * Handles the escape-code
 *
 * @param str the current position (will be changed)
 */
static void vterm_handleEscape(s8 **str);

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

static sVTerm vterm;

void vterm_init(void) {
	s32 vidFd,selfFd;
	u32 i;
	s8 *ptr;

	/* open video */
	do {
		vidFd = open("services:/video",IO_WRITE);
		if(vidFd < 0)
			yield();
	}
	while(vidFd < 0);

	/* open ourself to write into the receive-pipe (which can be read by other processes) */
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
	/* start on first line of the last page */
	vterm.firstLine = HISTORY_SIZE - ROWS;
	vterm.currLine = HISTORY_SIZE - ROWS;
	vterm.firstVisLine = HISTORY_SIZE - ROWS;

	/* build title bar */
	vterm.titleBar.header.id = MSG_VIDEO_SETSCREEN;
	vterm.titleBar.header.length = COLS * 2;
	ptr = vterm.titleBar.data;
	for(i = 0; i < COLS; i++) {
		*ptr++ = ' ';
		*ptr++ = 0x17;
	}
	i = (((COLS * 2) / 2) - (22 / 2)) & ~0x1;
	ptr = vterm.titleBar.data;
	*(ptr + i) = 'E';
	*(ptr + i + 2) = 's';
	*(ptr + i + 4) = 'c';
	*(ptr + i + 6) = 'a';
	*(ptr + i + 8) = 'p';
	*(ptr + i + 10) = 'e';
	*(ptr + i + 14) = 'v';
	*(ptr + i + 16) = '0';
	*(ptr + i + 18) = '.';
	*(ptr + i + 20) = '1';
	vterm_refreshScreen();
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

		switch(msg->keycode) {
			case VK_PGUP:
				vterm_scroll(ROWS);
				vterm_refreshScreen();
				break;
			case VK_PGDOWN:
				vterm_scroll(-ROWS);
				vterm_refreshScreen();
				break;
			case VK_UP:
				if(vterm.shiftDown) {
					vterm_scroll(1);
					vterm_refreshScreen();
				}
				break;
			case VK_DOWN:
				if(vterm.shiftDown) {
					vterm_scroll(-1);
					vterm_refreshScreen();
				}
				break;
		}

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
		vterm_newLine();
		vterm_refreshScreen();
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

static void vterm_newLine(void) {
	if(vterm.firstLine > 0) {
		/* move one line back */
		memmove(vterm.buffer.data + ((vterm.firstLine - 1) * COLS * 2),
				vterm.buffer.data + (vterm.firstLine * COLS * 2),
				(HISTORY_SIZE - vterm.firstLine) * COLS * 2);
		vterm.firstLine--;
	}
	else {
		/* overwrite first line */
		memmove(vterm.buffer.data + (vterm.firstLine * COLS * 2),
				vterm.buffer.data + ((vterm.firstLine + 1) * COLS * 2),
				(HISTORY_SIZE - vterm.firstLine) * COLS * 2);
	}

	/* clear last line */
	memset(vterm.buffer.data + (vterm.currLine + vterm.row - 1) * COLS * 2,0,COLS * 2);
}

static void vterm_scroll(s16 lines) {
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vterm.firstVisLine = MAX(vterm.firstLine,(s16)vterm.firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vterm.firstVisLine = MIN(HISTORY_SIZE - ROWS,vterm.firstVisLine - lines);
	}
}

static void vterm_refreshScreen(void) {
	u8 back[sizeof(sMsgDefHeader)];
	s8 *ptr = vterm.buffer.data + vterm.firstVisLine * COLS * 2;
	/* backup screen-data */
	memcpy(back,ptr - sizeof(sMsgDefHeader),sizeof(sMsgDefHeader));

	/* send message */
	sMsgDefHeader *header = (sMsgDefHeader*)(ptr - sizeof(sMsgDefHeader));
	header->id = MSG_VIDEO_SETSCREEN;
	header->length = ROWS * COLS * 2;
	write(vterm.video,ptr - sizeof(sMsgDefHeader),sizeof(sMsgDefHeader) + ROWS * COLS * 2);

	/* send message for titleBar */
	write(vterm.video,&vterm.titleBar,sizeof(vterm.titleBar));

	/* restore screen-data */
	memcpy(ptr - sizeof(sMsgDefHeader),back,sizeof(sMsgDefHeader));
}

static void vterm_writeChar(u8 c) {
	u8 color = (vterm.background << 4) | vterm.foreground;
	u32 i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);

	/* scroll to current line, if necessary */
	if(vterm.firstVisLine != vterm.currLine) {
		vterm_scroll(vterm.firstVisLine - vterm.currLine);
		vterm_refreshScreen();
	}

	/* write to buffer */
	vterm.buffer.data[i] = c;
	vterm.buffer.data[i + 1] = color;

	/* write to video-driver */
	msgVidSet.data.character = c;
	msgVidSet.data.color = color;
	msgVidSet.data.row = vterm.row;
	msgVidSet.data.col = vterm.col;
	write(vterm.video,&msgVidSet,sizeof(sMsgVidSet));
}

static void vterm_handleEscape(s8 **str) {
	u8 *fmt = (u8*)*str;
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
