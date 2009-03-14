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
#include "keymap.us.h"
#include "keymap.ger.h"

#define COLS				80
#define ROWS				25
#define TAB_WIDTH			2

/* the number of lines to keep in history */
#define HISTORY_SIZE		(ROWS * 8)
#define BUFFER_SIZE			(COLS * 2 * HISTORY_SIZE)

/* the number of left-shifts for each state */
#define STATE_SHIFT			0
#define STATE_CTRL			1
#define STATE_ALT			2

#define TITLE_BAR_COLOR		0x90
#define OS_TITLE			"E\x90" \
							"s\x90" \
							"c\x90" \
							"a\x90" \
							"p\x90" \
							"e\x90" \
							" \x90" \
							"v\x90" \
							"0\x90" \
							".\x90" \
							"1\x90"

typedef sKeymapEntry *(*keymapGetFunc)(u8 keyCode);

/* the header for the set-screen message */
typedef struct {
	sMsgDefHeader header;
	u16 startPos;
} __attribute__((packed)) sMsgVidSetScr;

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
	tFD speaker;
	tFD self;
	/* the first line with content */
	u16 firstLine;
	/* the line where row+col starts */
	u16 currLine;
	/* the first visible line */
	u16 firstVisLine;
	/* in message form for performance-issues */
	struct {
		sMsgVidSetScr header;
		s8 data[BUFFER_SIZE];
	} __attribute__((packed)) buffer;
	struct {
		sMsgVidSetScr header;
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
typedef struct {
	sMsgDefHeader header;
	sMsgDataSpeakerBeep data;
} __attribute__((packed)) sMsgSpeaker;

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
 * Refreshes <count> lines beginning with <start>
 *
 * @param start the start-line
 * @param count the number of lines
 */
static void vterm_refreshLines(u16 start,u16 count);

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
/* the speaker-message */
static sMsgSpeaker msgSpeaker = {
	.header = {
		.id = MSG_SPEAKER_BEEP,
		.length = sizeof(sMsgDataSpeakerBeep)
	},
	.data = {
		.frequency = 1000,
		.duration = 1
	}
};

/* our keymaps */
static u32 keymap = 1;
static keymapGetFunc keymaps[] = {
	keymap_us_get,
	keymap_ger_get
};

static sVTerm vterm;

void vterm_init(void) {
	s32 vidFd,selfFd,speakerFd;
	u32 i,len;
	s8 *ptr;

	/* open video */
	do {
		vidFd = open("services:/video",IO_WRITE);
		if(vidFd < 0)
			yield();
	}
	while(vidFd < 0);

	/* open speaker */
	do {
		speakerFd = open("services:/speaker",IO_WRITE);
		if(speakerFd < 0)
			yield();
	}
	while(speakerFd < 0);

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
	vterm.speaker = speakerFd;
	vterm.self = selfFd;
	/* start on first line of the last page */
	vterm.firstLine = HISTORY_SIZE - ROWS;
	vterm.currLine = HISTORY_SIZE - ROWS;
	vterm.firstVisLine = HISTORY_SIZE - ROWS;

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	memset(vterm.buffer.data,0x07200720,BUFFER_SIZE);

	/* build title bar */
	vterm.titleBar.header.header.id = MSG_VIDEO_SETSCREEN;
	vterm.titleBar.header.header.length = sizeof(u16) + COLS * 2;
	vterm.titleBar.header.startPos = 0;
	ptr = vterm.titleBar.data;
	for(i = 0; i < COLS; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((COLS * 2) / 2) - (len / 2)) & ~0x1;
	ptr = vterm.titleBar.data;
	memcpy(ptr + i,OS_TITLE,len);

	/* refresh screen and write titlebar (once) */
	vterm_refreshScreen();
	write(vterm.video,&vterm.titleBar,sizeof(vterm.titleBar));
}

void vterm_destroy(void) {
	close(vterm.video);
	close(vterm.speaker);
	close(vterm.self);
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

	e = keymaps[keymap](msg->keycode);
	if(e != NULL) {
		bool sendMsg = true;
		if(vterm.shiftDown)
			c = e->shift;
		else if(vterm.altDown)
			c = e->alt;
		else
			c = e->def;

		switch(msg->keycode) {
			case VK_PGUP:
				vterm_scroll(ROWS);
				sendMsg = false;
				break;
			case VK_PGDOWN:
				vterm_scroll(-ROWS);
				sendMsg = false;
				break;
			case VK_UP:
				if(vterm.shiftDown) {
					vterm_scroll(1);
					sendMsg = false;
				}
				break;
			case VK_DOWN:
				if(vterm.shiftDown) {
					vterm_scroll(-1);
					sendMsg = false;
				}
				break;
		}

		if(sendMsg) {
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
}

void vterm_puts(s8 *str) {
	s8 c;
	while((c = *str)) {
		if(c == '\033') {
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
		outByte(0xe9,c);
		outByte(0x3f8,c);
		while((inByte(0x3fd) & 0x20) == 0);
	}

	switch(c) {
		case '\n':
			/* to next line */
			vterm.row++;
			/* move cursor to line start */
			vterm_putchar('\r');
			break;

		case '\r':
			/* to line-start */
			vterm.col = 0;
			break;

		case '\a':
			/* beep */
			write(vterm.speaker,&msgSpeaker,sizeof(sMsgSpeaker));
			break;

		case '\b':
			if(vterm.col > 0) {
				i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);
				/* move the characters back in the buffer */
				memmove(vterm.buffer.data + i - 2,vterm.buffer.data + i,(COLS - vterm.col) * 2);
				vterm.col--;

				/* overwrite line */
				vterm_refreshLines(vterm.row,1);
				vterm_setCursor();
			}
			break;

		case '\t':
			i = TAB_WIDTH - vterm.col % TAB_WIDTH;
			while(i-- > 0) {
				vterm_putchar(' ');
			}
			break;

		default:
			vterm_writeChar(c);

			vterm.col++;
			/* do an explicit newline if necessary */
			if(vterm.col >= COLS)
				vterm_putchar('\n');
			break;
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
	memset(vterm.buffer.data + (vterm.currLine + vterm.row - 1) * COLS * 2,0x07200720,COLS * 2);
}

static void vterm_scroll(s16 lines) {
	u16 old = vterm.firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vterm.firstVisLine = MAX(vterm.firstLine,(s16)vterm.firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vterm.firstVisLine = MIN(HISTORY_SIZE - ROWS,vterm.firstVisLine - lines);
	}

	if(old != vterm.firstVisLine)
		vterm_refreshScreen();
}

static void vterm_refreshScreen(void) {
	vterm_refreshLines(1,ROWS - 1);
}

static void vterm_refreshLines(u16 start,u16 count) {
	u8 back[sizeof(sMsgVidSetScr)];
	s8 *ptr = vterm.buffer.data + (vterm.firstVisLine + start) * COLS * 2;
	/* backup screen-data */
	memcpy(back,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr));

	/* send message */
	sMsgVidSetScr *header = (sMsgVidSetScr*)(ptr - sizeof(sMsgVidSetScr));
	header->header.id = MSG_VIDEO_SETSCREEN;
	header->header.length = count * COLS * 2;
	header->startPos = start * COLS;
	write(vterm.video,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr) + count * COLS * 2);

	/* restore screen-data */
	memcpy(ptr - sizeof(sMsgVidSetScr),back,sizeof(sMsgVidSetScr));
}

static void vterm_writeChar(u8 c) {
	u8 color = (vterm.background << 4) | vterm.foreground;
	u32 i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);

	/* scroll to current line, if necessary */
	if(vterm.firstVisLine != vterm.currLine)
		vterm_scroll(vterm.firstVisLine - vterm.currLine);

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
