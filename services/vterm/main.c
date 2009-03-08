/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <service.h>
#include <io.h>
#include <heap.h>
#include <string.h>
#include <debug.h>
#include <proc.h>
#include <keycodes.h>

/* our vterm-state */
typedef struct {
	u8 col;
	u8 row;
	u8 foreground;
	u8 background;
	tFD video;
} sVTerm;

/* the message we'll send */
typedef struct {
	sMsgDefHeader header;
	sMsgDataVidSet data;
} __attribute__((packed)) sMsgVidSet;

#define COLS		80
#define ROWS		25
#define TAB_WIDTH	2

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Sets the cursor
 */
static void vterm_setCursor(u8 row,u8 col);

/**
 * Prints the given character to screen
 *
 * @param c the character
 */
static void vterm_putchar(s8 c);

/**
 * Prints the given string
 *
 * @param str the string
 */
static void vterm_puts(s8 *str);

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
/* the move-up message */
static sMsgDefHeader msgVidMove = {
	.id = MSG_VIDEO_MOVEUP,
	.length = 0
};

static u8 cursorCol = 0;
static u8 cursorRow = 0;
static sVTerm vterm;

s32 main(void) {
	s32 kbFd,vidFd;
	s32 id;

	debugf("Service vterm has pid %d\n",getpid());

	id = regService("vterm",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* open video */
	do {
		vidFd = open("services:/video",IO_WRITE);
		if(vidFd < 0)
			yield();
	}
	while(vidFd < 0);

	/* open keyboard */
	do {
		kbFd = open("services:/keyboard",IO_READ);
		if(kbFd < 0)
			yield();
	}
	while(kbFd < 0);

	/* request io-ports for qemu and bochs */
	requestIOPort(0xe9);
	requestIOPort(0x3f8);
	requestIOPort(0x3fd);
	requestIOPort(0x3d4);
	requestIOPort(0x3d5);

	/* init vterm */
	vterm.col = 0;
	vterm.row = ROWS - 1;
	vterm.foreground = WHITE;
	vterm.background = BLACK;
	vterm.video = vidFd;

	vterm_setCursor(cursorRow,cursorCol);

	static sMsgKbResponse keycode;
	static sMsgDefHeader msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0) {
			/* read from keyboard */
			while(read(kbFd,&keycode,sizeof(sMsgKbResponse)) > 0) {
				/*if(keycode.isBreak)
					debugf("Key %d released\n",keycode.keycode);
				else
					debugf("Key %d pressed\n",keycode.keycode);*/
				if(!keycode.isBreak) {
					switch(keycode.keycode) {
						case VK_LEFT:
							if(cursorCol > 0)
								cursorCol--;
							vterm_setCursor(cursorRow,cursorCol);
							break;
						case VK_RIGHT:
							if(cursorCol < COLS)
								cursorCol++;
							vterm_setCursor(cursorRow,cursorCol);
							break;
						case VK_UP:
							if(cursorRow > 0)
								cursorRow--;
							vterm_setCursor(cursorRow,cursorCol);
							break;
						case VK_DOWN:
							if(cursorRow < ROWS)
								cursorRow++;
							vterm_setCursor(cursorRow,cursorCol);
							break;
					}
				}
			}
			yield();
		}
		else {
			s32 x = 0,c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sMsgDefHeader))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == MSG_VTERM_WRITE) {
						if(msg.length > 0) {
							s8 *buffer = malloc(msg.length * sizeof(s8));
							read(fd,buffer,msg.length);
							*(buffer + msg.length - 1) = '\0';
							vterm_puts(buffer);
							free(buffer);
						}
					}
					else if(msg.id == MSG_VTERM_READ) {

					}
					x++;
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);
	releaseIOPort(0x3d4);
	releaseIOPort(0x3d5);
	close(kbFd);
	unregService(id);
	return 0;
}

static void vterm_setCursor(u8 row,u8 col) {
   u16 position = (row * COLS) + col;

   /* cursor LOW port to vga INDEX register */
   outb(0x3D4,0x0F);
   outb(0x3D5,(u8)(position & 0xFF));
   /* cursor HIGH port to vga INDEX register */
   outb(0x3D4,0x0E);
   outb(0x3D5,(u8)((position >> 8) & 0xFF));
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

static void vterm_puts(s8 *str) {
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
