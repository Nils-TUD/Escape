/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <io.h>
#include <ports.h>
#include <proc.h>
#include <keycodes.h>
#include <heap.h>
#include <string.h>
#include <signals.h>
#include "vterm.h"
#include "keymap.h"
#include "keymap.us.h"
#include "keymap.ger.h"

#define COLS				80
#define ROWS				25
#define TAB_WIDTH			4

/* the number of lines to keep in history */
#define HISTORY_SIZE		(ROWS * 8)
#define BUFFER_SIZE			(COLS * 2 * HISTORY_SIZE)
#define INITIAL_RLBUF_SIZE	50
#define RLBUF_INCR			20

/* the number of left-shifts for each state */
#define STATE_SHIFT			0
#define STATE_CTRL			1
#define STATE_ALT			2

#define TITLE_BAR_COLOR		0x90
#define OS_TITLE			"E\x97" \
							"s\x97" \
							"c\x97" \
							"a\x97" \
							"p\x97" \
							"e\x97" \
							" \x97" \
							"v\x97" \
							"0\x97" \
							".\x97" \
							"1\x97"

typedef sKeymapEntry *(*fKeymapGet)(u8 keyCode);

/* the header for the set-screen message */
typedef struct {
	sMsgHeader header;
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
	/* the used keymap */
	u16 keymap;
	/* wether entered characters should be echo'd to screen */
	bool echo;
	/* wether the vterm should read until a newline occurrs */
	bool readLine;
	/* readline-buffer */
	u8 rlStartCol;
	u32 rlBufSize;
	u32 rlBufPos;
	char *rlBuffer;
	/* in message form for performance-issues */
	struct {
		sMsgVidSetScr header;
		char data[BUFFER_SIZE];
	} __attribute__((packed)) buffer;
	struct {
		sMsgVidSetScr header;
		char data[COLS * 2];
	} __attribute__((packed)) titleBar;
} sVTerm;

/* the messages we'll send */
typedef struct {
	sMsgHeader header;
	sMsgDataVidSet data;
} __attribute__((packed)) sMsgVidSet;
typedef struct {
	sMsgHeader header;
	sMsgDataVidSetCursor data;
} __attribute__((packed)) sMsgVidSetCursor;
typedef struct {
	sMsgHeader header;
	sMsgDataSpeakerBeep data;
} __attribute__((packed)) sMsgSpeaker;

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Sends the character at given position to the video-driver
 *
 * @param row the row
 * @param col the col
 */
static void vterm_sendChar(u8 row,u8 col);

/**
 * Sets the cursor
 */
static void vterm_setCursor(void);

/**
 * Prints the given character to screen
 *
 * @param c the character
 */
static void vterm_putchar(char c);

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
 * @param str the current position
 * @return true if something has been done
 */
static bool vterm_handleEscape(char *str);

/**
 * Flushes the readline-buffer
 */
static void vterm_rlFlushBuf(void);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary
 *
 * @param c the character
 */
static void vterm_rlPutchar(char c);

/**
 * @return the current position in the readline-buffer
 */
static u32 vterm_rlGetBufPos(void);

/**
 * Handles the given keycode for readline
 *
 * @param keycode the keycode
 * @return true if handled
 */
static bool vterm_rlHandleKeycode(u8 keycode);

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
static fKeymapGet keymaps[] = {
	keymap_us_get,
	keymap_ger_get
};

static sVTerm vterm;

bool vterm_init(void) {
	tFD vidFd,selfFd,speakerFd;
	u32 i,len;
	char *ptr;

	/* open video */
	vidFd = open("services:/video",IO_WRITE);
	if(vidFd < 0) {
		printLastError();
		return false;
	}

	/* open speaker */
	speakerFd = open("services:/speaker",IO_WRITE);
	if(speakerFd < 0) {
		printLastError();
		return false;
	}

	/* open ourself to write into the receive-pipe (which can be read by other processes) */
	selfFd = open("services:/vterm",IO_WRITE);
	if(selfFd < 0) {
		printLastError();
		return false;
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
	/* default behaviour */
	vterm.echo = true;
	vterm.readLine = true;
	vterm.keymap = 1;
	vterm.rlStartCol = 0;
	vterm.rlBufSize = INITIAL_RLBUF_SIZE;
	vterm.rlBufPos = 0;
	vterm.rlBuffer = (char*)malloc(vterm.rlBufSize * sizeof(char));
	if(vterm.rlBuffer == NULL) {
		printLastError();
		return false;
	}

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
	return true;
}

void vterm_destroy(void) {
	close(vterm.video);
	close(vterm.speaker);
	close(vterm.self);
}

void vterm_puts(char *str,bool resetRead) {
	char c;
	u32 oldFirstLine = vterm.firstLine;
	u32 newPos,oldPos = vterm.row * COLS + vterm.col;
	u32 start,count;

	while((c = *str)) {
		if(c == '\033') {
			str++;
			vterm_handleEscape(str);
			str += 2;
			continue;
		}
		vterm_putchar(c);
		str++;
	}

	/* scroll to current line, if necessary */
	if(vterm.firstVisLine != vterm.currLine)
		vterm_scroll(vterm.firstVisLine - vterm.currLine);

	/* so refresh all lines that need to be refreshed. thats faster than sending all
	 * chars individually */
	newPos = vterm.row * COLS + vterm.col;
	start = oldPos / COLS;
	count = ((newPos - oldPos) + COLS - 1) / COLS;
	count += oldFirstLine - vterm.firstLine;
	if(count <= 0)
		count = 1;
	vterm_refreshLines(start,count);
	vterm_setCursor();

	/* reset reading */
	if(resetRead) {
		vterm.rlBufPos = 0;
		vterm.rlStartCol = vterm.col;
	}
}

static void vterm_sendChar(u8 row,u8 col) {
	char *ptr = vterm.buffer.data + (vterm.currLine * COLS * 2) + (row * COLS * 2) + (col * 2);
	u8 color = *(ptr + 1);

	/* scroll to current line, if necessary */
	if(vterm.firstVisLine != vterm.currLine)
		vterm_scroll(vterm.firstVisLine - vterm.currLine);

	/* write last character to video-driver */
	msgVidSet.data.character = *ptr;
	msgVidSet.data.color = color;
	msgVidSet.data.row = row;
	msgVidSet.data.col = col;
	write(vterm.video,&msgVidSet,sizeof(sMsgVidSet));
}

static void vterm_setCursor(void) {
	msgVidSetCursor.data.col = vterm.col;
	msgVidSetCursor.data.row = vterm.row;
	write(vterm.video,&msgVidSetCursor,sizeof(sMsgVidSetCursor));
}

static void vterm_putchar(char c) {
	u32 i;

	/* move all one line up, if necessary */
	if(vterm.row >= ROWS) {
		vterm_newLine();
		vterm_refreshScreen();
		vterm.row--;
	}

	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r' && c != '\a' && c != '\b') {
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
			if((!vterm.readLine && vterm.col > 0) || (vterm.readLine && vterm.rlBufPos > 0)) {
				if(!vterm.readLine || vterm.echo) {
					i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);
					/* move the characters back in the buffer */
					memmove(vterm.buffer.data + i - 2,vterm.buffer.data + i,(COLS - vterm.col) * 2);
					vterm.col--;
				}

				if(vterm.readLine) {
					vterm.rlBuffer[vterm.rlBufPos] = '\0';
					vterm.rlBufPos--;
				}

				/* overwrite line */
				vterm_refreshLines(vterm.row,1);
				vterm_setCursor();
			}
			else {
				/* beep */
				write(vterm.speaker,&msgSpeaker,sizeof(sMsgSpeaker));
			}
			break;

		case '\t':
			i = TAB_WIDTH - vterm.col % TAB_WIDTH;
			while(i-- > 0) {
				vterm_putchar(' ');
			}
			break;

		default: {
			i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);

			/* write to buffer */
			vterm.buffer.data[i] = c;
			vterm.buffer.data[i + 1] = (vterm.background << 4) | vterm.foreground;

			vterm.col++;
			/* do an explicit newline if necessary */
			if(vterm.col >= COLS)
				vterm_putchar('\n');
		}
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
	char *ptr = vterm.buffer.data + (vterm.firstVisLine + start) * COLS * 2;
	/* backup screen-data */
	memcpy(back,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr));

	/* send message */
	sMsgVidSetScr *header = (sMsgVidSetScr*)(ptr - sizeof(sMsgVidSetScr));
	header->header.id = MSG_VIDEO_SETSCREEN;
	header->header.length = (sizeof(sMsgVidSetScr) - sizeof(sMsgHeader)) + count * COLS * 2;
	header->startPos = start * COLS;
	write(vterm.video,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr) + count * COLS * 2);

	/* restore screen-data */
	memcpy(ptr - sizeof(sMsgVidSetScr),back,sizeof(sMsgVidSetScr));
}

static bool vterm_handleEscape(char *str) {
	u8 keycode = *str;
	u8 value = *(str + 1);
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(vterm.col > 0) {
				vterm.col--;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_RIGHT:
			if(vterm.col < COLS - 1) {
				vterm.col++;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_HOME:
			if(value > 0) {
				if(value > vterm.col)
					vterm.col = 0;
				else
					vterm.col -= value;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_END:
			if(value > 0) {
				if(vterm.col + value > COLS - 1)
					vterm.col = COLS - 1;
				else
					vterm.col += value;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_ESC_RESET:
			vterm.foreground = WHITE;
			vterm.background = BLACK;
			res = true;
			break;
		case VK_ESC_FG:
			vterm.foreground = MIN(9,value);
			res = true;
			break;
		case VK_ESC_BG:
			vterm.background = MIN(9,value);
			res = true;
			break;
		case VK_ESC_SET_ECHO:
			vterm.echo = value ? true : false;
			res = true;
			break;
		case VK_ESC_SET_RL:
			vterm.readLine = value ? true : false;
			if(vterm.readLine) {
				/* reset reading */
				vterm.rlBufPos = 0;
				vterm.rlStartCol = vterm.col;
			}
			res = true;
			break;
	}

	return res;
}

void vterm_handleKeycode(sMsgKbResponse *msg) {
	sKeymapEntry *e;
	char c;

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

	e = keymaps[vterm.keymap](msg->keycode);
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
			if(c == NPRINT || vterm.ctrlDown) {
				/* handle ^C, ^D and so on */
				if(vterm.ctrlDown) {
					switch(msg->keycode) {
						case VK_C:
							sendSignal(SIG_INTRPT,0);
							break;
						case VK_D:
							if(vterm.readLine)
								vterm_rlPutchar('\n');
							break;
					}
				}

				/* in reading mode? */
				if(vterm.readLine) {
					if(vterm.echo)
						vterm_rlHandleKeycode(msg->keycode);
				}
				/* send escape-code */
				else {
					char escape[3] = {'\033',msg->keycode,(vterm.altDown << STATE_ALT) |
							(vterm.ctrlDown << STATE_CTRL) |
							(vterm.shiftDown << STATE_SHIFT)};
					write(vterm.self,&escape,sizeof(char) * 3);
				}
			}
			else {
				if(vterm.readLine)
					vterm_rlPutchar(c);
				else
					write(vterm.self,&c,sizeof(char));
			}
		}
	}
}

static void vterm_rlFlushBuf(void) {
	u32 bufPos = vterm_rlGetBufPos();
	if(vterm.echo)
		bufPos++;

	if(bufPos > 0) {
		write(vterm.self,vterm.rlBuffer,bufPos * sizeof(char));
		vterm.rlBufPos = 0;
	}
}

static void vterm_rlPutchar(char c) {
	switch(c) {
		case '\b': {
			u32 bufPos = vterm_rlGetBufPos();
			if(bufPos > 0) {
				if(vterm.echo) {
					u32 i = (vterm.currLine * COLS * 2) + (vterm.row * COLS * 2) + (vterm.col * 2);
					/* move the characters back in the buffer */
					memmove(vterm.buffer.data + i - 2,vterm.buffer.data + i,(COLS - vterm.col) * 2);
					vterm.col--;
				}

				/* move chars back */
				memmove(vterm.rlBuffer + bufPos - 1,vterm.rlBuffer + bufPos,vterm.rlBufPos - bufPos);
				/* remove last */
				vterm.rlBuffer[vterm.rlBufPos - 1] = '\0';
				vterm.rlBufPos--;

				/* overwrite line */
				vterm_refreshLines(vterm.row,1);
				vterm_setCursor();
			}
		}
		break;

		case '\r':
		case '\a':
		case '\t':
			/* ignore */
			break;

		default: {
			bool flushed = false;
			bool moved = false;
			u32 bufPos = vterm_rlGetBufPos();

			/* increase buffer size? */
			if(vterm.rlBuffer && bufPos >= vterm.rlBufSize) {
				vterm.rlBufSize += RLBUF_INCR;
				vterm.rlBuffer = (char*)realloc(vterm.rlBuffer,vterm.rlBufSize);
			}
			if(vterm.rlBuffer == NULL)
				return;

			/* move chars forward */
			if(bufPos < vterm.rlBufPos) {
				memmove(vterm.rlBuffer + bufPos + 1,vterm.rlBuffer + bufPos,vterm.rlBufPos - bufPos);
				moved = true;
			}

			/** add char */
			vterm.rlBuffer[bufPos] = c;
			vterm.rlBufPos++;

			/* TODO later we should allow "multiline-editing" */
			if(c == '\n' || vterm.rlStartCol + vterm.rlBufPos >= COLS) {
				flushed = true;
				vterm_rlFlushBuf();
			}

			/* echo character, if required */
			if(vterm.echo) {
				if(moved && !flushed) {
					u32 count = vterm.rlBufPos - bufPos + 1;
					char *copy = (char*)malloc(count * sizeof(char));
					if(copy != NULL) {
						/* print the end of the buffer again */
						strncpy(copy,vterm.rlBuffer + bufPos,count - 1);
						copy[count - 1] = '\0';
						vterm_puts(copy,false);
						free(copy);

						/* reset cursor */
						vterm.col = vterm.rlStartCol + bufPos + 1;
						vterm_setCursor();
					}
				}
				else {
					u8 oldRow = vterm.row,oldCol = vterm.col;
					vterm_putchar(c);
					if(vterm.row != oldRow || vterm.col != oldCol)
						vterm_sendChar(oldRow,oldCol);
				}
			}
			if(flushed)
				vterm.rlStartCol = vterm.col;
		}
		break;
	}
}

static u32 vterm_rlGetBufPos(void) {
	if(vterm.echo)
		return vterm.col - vterm.rlStartCol;
	else
		return vterm.rlBufPos;
}

static bool vterm_rlHandleKeycode(u8 keycode) {
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(vterm.col > vterm.rlStartCol) {
				vterm.col--;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_RIGHT:
			if(vterm.col < vterm.rlStartCol + vterm.rlBufPos) {
				vterm.col++;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_HOME:
			if(vterm.col != vterm.rlStartCol) {
				vterm.col = vterm.rlStartCol;
				vterm_setCursor();
			}
			res = true;
			break;
		case VK_END:
			if(vterm.col != vterm.rlStartCol + vterm.rlBufPos) {
				vterm.col = vterm.rlStartCol + vterm.rlBufPos;
				vterm_setCursor();
			}
			res = true;
			break;
	}
	return res;
}
