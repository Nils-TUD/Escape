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
#include <bufio.h>
#include "vterm.h"
#include "keymap.h"
#include "keymap.us.h"
#include "keymap.ger.h"

/* the number of lines to keep in history */
#define INITIAL_RLBUF_SIZE	50
#define RLBUF_INCR			20

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

typedef sKeymapEntry *(*fKeymapGet)(u8 keyCode);

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
 * Inits the vterm
 *
 * @param vt the vterm
 * @return true if successfull
 */
static bool vterm_init(sVTerm *vt);

/**
 * Sends the character at given position to the video-driver
 *
 * @param vt the vterm
 * @param row the row
 * @param col the col
 */
static void vterm_sendChar(sVTerm *vt,u8 row,u8 col);

/**
 * Sets the cursor
 *
 * @param vt the vterm
 */
static void vterm_setCursor(sVTerm *vt);

/**
 * Prints the given character to screen
 *
 * @param vt the vterm
 * @param c the character
 */
static void vterm_putchar(sVTerm *vt,char c);

/**
 * Inserts a new line
 *
 * @param vt the vterm
 */
static void vterm_newLine(sVTerm *vt);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param vt the vterm
 * @param lines the number of lines to move
 */
static void vterm_scroll(sVTerm *vt,s16 lines);

/**
 * Refreshes the screen
 *
 * @param vt the vterm
 */
static void vterm_refreshScreen(sVTerm *vt);

/**
 * Refreshes <count> lines beginning with <start>
 *
 * @param vt the vterm
 * @param start the start-line
 * @param count the number of lines
 */
static void vterm_refreshLines(sVTerm *vt,u16 start,u16 count);

/**
 * Handles the escape-code
 *
 * @param vt the vterm
 * @param str the current position
 * @return true if something has been done
 */
static bool vterm_handleEscape(sVTerm *vt,char *str);

/**
 * Flushes the readline-buffer
 *
 * @param vt the vterm
 */
static void vterm_rlFlushBuf(sVTerm *vt);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary
 *
 * @param vt the vterm
 * @param c the character
 */
static void vterm_rlPutchar(sVTerm *vt,char c);

/**
 * @param vt the vterm
 * @return the current position in the readline-buffer
 */
static u32 vterm_rlGetBufPos(sVTerm *vt);

/**
 * Handles the given keycode for readline
 *
 * @param vt the vterm
 * @param keycode the keycode
 * @return true if handled
 */
static bool vterm_rlHandleKeycode(sVTerm *vt,u8 keycode);

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
/* vterms */
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;

/* key states */
static bool shiftDown;
static bool altDown;
static bool ctrlDown;

bool vterm_initAll(void) {
	char name[MAX_NAME_LEN + 1];
	u32 i;

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		sprintf(name,"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_NAME_LEN + 1);
		if(!vterm_init(vterms + i))
			return false;
	}

	shiftDown = false;
	altDown = false;
	ctrlDown = false;
	return true;
}

sVTerm *vterm_get(u32 index) {
	return vterms + index;
}

static bool vterm_init(sVTerm *vt) {
	tFD vidFd,selfFd,speakerFd;
	u32 i,len;
	char path[strlen("services:/") + MAX_NAME_LEN + 1];
	char *ptr,*s;

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
	sprintf(path,"services:/%s",vt->name);
	selfFd = open(path,IO_WRITE);
	if(selfFd < 0) {
		printLastError();
		return false;
	}

	/* init state */
	vt->col = 0;
	vt->row = ROWS - 1;
	vt->foreground = WHITE;
	vt->background = BLACK;
	vt->active = false;
	vt->video = vidFd;
	vt->speaker = speakerFd;
	vt->self = selfFd;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE - ROWS;
	vt->currLine = HISTORY_SIZE - ROWS;
	vt->firstVisLine = HISTORY_SIZE - ROWS;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->keymap = 1;
	vt->rlStartCol = 0;
	vt->rlBufSize = INITIAL_RLBUF_SIZE;
	vt->rlBufPos = 0;
	vt->rlBuffer = (char*)malloc(vt->rlBufSize * sizeof(char));
	if(vt->rlBuffer == NULL) {
		printLastError();
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	memset(vt->buffer.data,0x07200720,BUFFER_SIZE);

	/* build title bar */
	vt->titleBar.header.header.id = MSG_VIDEO_SETSCREEN;
	vt->titleBar.header.header.length = sizeof(u16) + COLS * 2;
	vt->titleBar.header.startPos = 0;
	ptr = vt->titleBar.data;
	s = vt->name;
	for(i = 0; *s; i++) {
		*ptr++ = *s++;
		*ptr++ = TITLE_BAR_COLOR;
	}
	for(; i < COLS; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((COLS * 2) / 2) - (len / 2)) & ~0x1;
	ptr = vt->titleBar.data;
	memcpy(ptr + i,OS_TITLE,len);
	return true;
}

void vterm_selectVTerm(u32 index) {
	sVTerm *vt = vterms + index;
	if(activeVT != NULL)
		activeVT->active = false;
	vt->active = true;
	activeVT = vt;

	/* refresh screen and write titlebar */
	write(vt->video,&vt->titleBar,sizeof(vt->titleBar));
	vterm_refreshScreen(vt);
	vterm_setCursor(vt);
}

void vterm_destroy(sVTerm *vt) {
	free(vt->rlBuffer);
	close(vt->video);
	close(vt->speaker);
	close(vt->self);
}

void vterm_puts(sVTerm *vt,char *str,bool resetRead) {
	char c;
	u32 oldFirstLine = vt->firstLine;
	u32 newPos,oldPos = vt->row * COLS + vt->col;
	u32 start,count;

	while((c = *str)) {
		if(c == '\033') {
			str++;
			vterm_handleEscape(vt,str);
			str += 2;
			continue;
		}
		vterm_putchar(vt,c);
		str++;
	}

	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vterm_scroll(vt,vt->firstVisLine - vt->currLine);

	if(vt->active) {
		/* so refresh all lines that need to be refreshed. thats faster than sending all
		 * chars individually */
		newPos = vt->row * COLS + vt->col;
		start = oldPos / COLS;
		count = ((newPos - oldPos) + COLS - 1) / COLS;
		count += oldFirstLine - vt->firstLine;
		if(count <= 0)
			count = 1;
		vterm_refreshLines(vt,start,count);
		vterm_setCursor(vt);
	}

	/* reset reading */
	if(resetRead) {
		vt->rlBufPos = 0;
		vt->rlStartCol = vt->col;
	}
}

static void vterm_sendChar(sVTerm *vt,u8 row,u8 col) {
	char *ptr = vt->buffer.data + (vt->currLine * COLS * 2) + (row * COLS * 2) + (col * 2);
	u8 color = *(ptr + 1);

	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vterm_scroll(vt,vt->firstVisLine - vt->currLine);

	/* write last character to video-driver */
	if(vt->active) {
		msgVidSet.data.character = *ptr;
		msgVidSet.data.color = color;
		msgVidSet.data.row = row;
		msgVidSet.data.col = col;
		write(vt->video,&msgVidSet,sizeof(sMsgVidSet));
	}
}

static void vterm_setCursor(sVTerm *vt) {
	if(vt->active) {
		msgVidSetCursor.data.col = vt->col;
		msgVidSetCursor.data.row = vt->row;
		write(vt->video,&msgVidSetCursor,sizeof(sMsgVidSetCursor));
	}
}

static void vterm_putchar(sVTerm *vt,char c) {
	u32 i;

	/* move all one line up, if necessary */
	if(vt->row >= ROWS) {
		vterm_newLine(vt);
		vterm_refreshScreen(vt);
		vt->row--;
	}

	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r' && c != '\a' && c != '\b' && c != '\t') {
		outByte(0xe9,c);
		outByte(0x3f8,c);
		while((inByte(0x3fd) & 0x20) == 0);
	}

	switch(c) {
		case '\n':
			/* to next line */
			vt->row++;
			/* move cursor to line start */
			vterm_putchar(vt,'\r');
			break;

		case '\r':
			/* to line-start */
			vt->col = 0;
			break;

		case '\a':
			/* beep */
			write(vt->speaker,&msgSpeaker,sizeof(sMsgSpeaker));
			break;

		case '\b':
			if((!vt->readLine && vt->col > 0) || (vt->readLine && vt->rlBufPos > 0)) {
				if(!vt->readLine || vt->echo) {
					i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);
					/* move the characters back in the buffer */
					memmove(vt->buffer.data + i - 2,vt->buffer.data + i,(COLS - vt->col) * 2);
					vt->col--;
				}

				if(vt->readLine) {
					vt->rlBuffer[vt->rlBufPos] = '\0';
					vt->rlBufPos--;
				}

				/* overwrite line */
				vterm_refreshLines(vt,vt->row,1);
				vterm_setCursor(vt);
			}
			else {
				/* beep */
				write(vt->speaker,&msgSpeaker,sizeof(sMsgSpeaker));
			}
			break;

		case '\t':
			i = TAB_WIDTH - vt->col % TAB_WIDTH;
			while(i-- > 0) {
				vterm_putchar(vt,' ');
			}
			break;

		default: {
			/* do an explicit newline if necessary */
			if(vt->col >= COLS)
				vterm_putchar(vt,'\n');

			i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);

			/* write to buffer */
			vt->buffer.data[i] = c;
			vt->buffer.data[i + 1] = (vt->background << 4) | vt->foreground;

			vt->col++;
		}
		break;
	}
}

static void vterm_newLine(sVTerm *vt) {
	if(vt->firstLine > 0) {
		/* move one line back */
		memmove(vt->buffer.data + ((vt->firstLine - 1) * COLS * 2),
				vt->buffer.data + (vt->firstLine * COLS * 2),
				(HISTORY_SIZE - vt->firstLine) * COLS * 2);
		vt->firstLine--;
	}
	else {
		/* overwrite first line */
		memmove(vt->buffer.data + (vt->firstLine * COLS * 2),
				vt->buffer.data + ((vt->firstLine + 1) * COLS * 2),
				(HISTORY_SIZE - vt->firstLine) * COLS * 2);
	}

	/* clear last line */
	memset(vt->buffer.data + (vt->currLine + vt->row - 1) * COLS * 2,0x07200720,COLS * 2);
}

static void vterm_scroll(sVTerm *vt,s16 lines) {
	u16 old = vt->firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vt->firstVisLine = MAX(vt->firstLine,(s16)vt->firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vt->firstVisLine = MIN(HISTORY_SIZE - ROWS,vt->firstVisLine - lines);
	}

	if(vt->active && old != vt->firstVisLine)
		vterm_refreshScreen(vt);
}

static void vterm_refreshScreen(sVTerm *vt) {
	vterm_refreshLines(vt,1,ROWS - 1);
}

static void vterm_refreshLines(sVTerm *vt,u16 start,u16 count) {
	u8 back[sizeof(sMsgVidSetScr)];
	char *ptr;

	if(!vt->active)
		return;

	/* backup screen-data */
	ptr = vt->buffer.data + (vt->firstVisLine + start) * COLS * 2;
	memcpy(back,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr));

	/* send message */
	sMsgVidSetScr *header = (sMsgVidSetScr*)(ptr - sizeof(sMsgVidSetScr));
	header->header.id = MSG_VIDEO_SETSCREEN;
	header->header.length = (sizeof(sMsgVidSetScr) - sizeof(sMsgHeader)) + count * COLS * 2;
	header->startPos = start * COLS;
	write(vt->video,ptr - sizeof(sMsgVidSetScr),sizeof(sMsgVidSetScr) + count * COLS * 2);

	/* restore screen-data */
	memcpy(ptr - sizeof(sMsgVidSetScr),back,sizeof(sMsgVidSetScr));
}

static bool vterm_handleEscape(sVTerm *vt,char *str) {
	u8 keycode = *str;
	u8 value = *(str + 1);
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(vt->col > 0) {
				vt->col--;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_RIGHT:
			if(vt->col < COLS - 1) {
				vt->col++;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_HOME:
			if(value > 0) {
				if(value > vt->col)
					vt->col = 0;
				else
					vt->col -= value;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_END:
			if(value > 0) {
				if(vt->col + value > COLS - 1)
					vt->col = COLS - 1;
				else
					vt->col += value;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_ESC_RESET:
			vt->foreground = WHITE;
			vt->background = BLACK;
			res = true;
			break;
		case VK_ESC_FG:
			vt->foreground = MIN(9,value);
			res = true;
			break;
		case VK_ESC_BG:
			vt->background = MIN(9,value);
			res = true;
			break;
		case VK_ESC_SET_ECHO:
			vt->echo = value ? true : false;
			res = true;
			break;
		case VK_ESC_SET_RL:
			vt->readLine = value ? true : false;
			if(vt->readLine) {
				/* reset reading */
				vt->rlBufPos = 0;
				vt->rlStartCol = vt->col;
			}
			res = true;
			break;
	}

	return res;
}

void vterm_handleKeycode(sMsgKbResponse *msg) {
	sKeymapEntry *e;
	sVTerm *vt = activeVT;
	char c;

	if(vt == NULL)
		return;

	/* handle shift, alt and ctrl */
	switch(msg->keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			shiftDown = !msg->isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			altDown = !msg->isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			ctrlDown = !msg->isBreak;
			break;
	}

	/* we don't need breakcodes anymore */
	if(msg->isBreak)
		return;

	e = keymaps[vt->keymap](msg->keycode);
	if(e != NULL) {
		bool sendMsg = true;
		if(shiftDown)
			c = e->shift;
		else if(altDown)
			c = e->alt;
		else
			c = e->def;

		switch(msg->keycode) {
			case VK_PGUP:
				vterm_scroll(vt,ROWS);
				sendMsg = false;
				break;
			case VK_PGDOWN:
				vterm_scroll(vt,-ROWS);
				sendMsg = false;
				break;
			case VK_UP:
				if(shiftDown) {
					vterm_scroll(vt,1);
					sendMsg = false;
				}
				break;
			case VK_DOWN:
				if(shiftDown) {
					vterm_scroll(vt,-1);
					sendMsg = false;
				}
				break;
		}

		if(sendMsg) {
			if(c == NPRINT || ctrlDown) {
				/* handle ^C, ^D and so on */
				if(ctrlDown) {
					switch(msg->keycode) {
						case VK_C:
							sendSignal(SIG_INTRPT,activeVT->index);
							break;
						case VK_D:
							if(vt->readLine) {
								vterm_rlPutchar(vt,IO_EOF);
								vterm_rlFlushBuf(vt);
							}
							break;
						case VK_1:
							if(activeVT->index != 0)
								vterm_selectVTerm(0);
							return;
						case VK_2:
							if(activeVT->index != 1)
								vterm_selectVTerm(1);
							return;
					}
				}

				/* in reading mode? */
				if(vt->readLine) {
					if(vt->echo)
						vterm_rlHandleKeycode(vt,msg->keycode);
				}
				/* send escape-code */
				else {
					char escape[3] = {'\033',msg->keycode,(altDown << STATE_ALT) |
							(ctrlDown << STATE_CTRL) |
							(shiftDown << STATE_SHIFT)};
					write(vt->self,&escape,sizeof(char) * 3);
				}
			}
			else {
				if(vt->readLine)
					vterm_rlPutchar(vt,c);
				else
					write(vt->self,&c,sizeof(char));
			}
		}
	}
}

static void vterm_rlFlushBuf(sVTerm *vt) {
	u32 bufPos = vterm_rlGetBufPos(vt);
	if(vt->echo)
		bufPos++;

	if(bufPos > 0) {
		write(vt->self,vt->rlBuffer,bufPos * sizeof(char));
		vt->rlBufPos = 0;
	}
}

static void vterm_rlPutchar(sVTerm *vt,char c) {
	switch(c) {
		case '\b': {
			u32 bufPos = vterm_rlGetBufPos(vt);
			if(bufPos > 0) {
				if(vt->echo) {
					u32 i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);
					/* move the characters back in the buffer */
					memmove(vt->buffer.data + i - 2,vt->buffer.data + i,(COLS - vt->col) * 2);
					vt->col--;
				}

				/* move chars back */
				memmove(vt->rlBuffer + bufPos - 1,vt->rlBuffer + bufPos,vt->rlBufPos - bufPos);
				/* remove last */
				vt->rlBuffer[vt->rlBufPos - 1] = '\0';
				vt->rlBufPos--;

				/* overwrite line */
				vterm_refreshLines(vt,vt->row,1);
				vterm_setCursor(vt);
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
			u32 bufPos = vterm_rlGetBufPos(vt);

			/* increase buffer size? */
			if(vt->rlBuffer && bufPos >= vt->rlBufSize) {
				vt->rlBufSize += RLBUF_INCR;
				vt->rlBuffer = (char*)realloc(vt->rlBuffer,vt->rlBufSize);
			}
			if(vt->rlBuffer == NULL)
				return;

			/* move chars forward */
			if(bufPos < vt->rlBufPos) {
				memmove(vt->rlBuffer + bufPos + 1,vt->rlBuffer + bufPos,vt->rlBufPos - bufPos);
				moved = true;
			}

			/** add char */
			vt->rlBuffer[bufPos] = c;
			vt->rlBufPos++;

			/* TODO later we should allow "multiline-editing" */
			if(c == '\n' || vt->rlStartCol + vt->rlBufPos >= COLS) {
				flushed = true;
				vterm_rlFlushBuf(vt);
			}

			/* echo character, if required */
			if(vt->echo) {
				if(moved && !flushed) {
					u32 count = vt->rlBufPos - bufPos + 1;
					char *copy = (char*)malloc(count * sizeof(char));
					if(copy != NULL) {
						/* print the end of the buffer again */
						strncpy(copy,vt->rlBuffer + bufPos,count - 1);
						copy[count - 1] = '\0';
						vterm_puts(vt,copy,false);
						free(copy);

						/* reset cursor */
						vt->col = vt->rlStartCol + bufPos + 1;
						vterm_setCursor(vt);
					}
				}
				else if(c != IO_EOF) {
					u8 oldRow = vt->row,oldCol = vt->col;
					vterm_putchar(vt,c);
					if(vt->row != oldRow || vt->col != oldCol)
						vterm_sendChar(vt,oldRow,oldCol);
					vterm_setCursor(vt);
				}
			}
			if(flushed)
				vt->rlStartCol = vt->col;
		}
		break;
	}
}

static u32 vterm_rlGetBufPos(sVTerm *vt) {
	if(vt->echo)
		return vt->col - vt->rlStartCol;
	else
		return vt->rlBufPos;
}

static bool vterm_rlHandleKeycode(sVTerm *vt,u8 keycode) {
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(vt->col > vt->rlStartCol) {
				vt->col--;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_RIGHT:
			if(vt->col < vt->rlStartCol + vt->rlBufPos) {
				vt->col++;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_HOME:
			if(vt->col != vt->rlStartCol) {
				vt->col = vt->rlStartCol;
				vterm_setCursor(vt);
			}
			res = true;
			break;
		case VK_END:
			if(vt->col != vt->rlStartCol + vt->rlBufPos) {
				vt->col = vt->rlStartCol + vt->rlBufPos;
				vterm_setCursor(vt);
			}
			res = true;
			break;
	}
	return res;
}
