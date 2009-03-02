/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <dir.h>
#include <string.h>
#include <service.h>
#include <mem.h>
#include <heap.h>

#include <test.h>
#include <theap.h>
#include <sllist.h>

#define COL_WOB		0x07				/* white on black */
#define COLS		80
#define ROWS		25
#define TAB_WIDTH	2

#define ID_OUT		0
#define ID_IN		1
#define ID_CLEAR	2

static s8 *videoBase = NULL;
static s8 *video = NULL;
static s8 hexCharsBig[] = "0123456789ABCDEF";
static s8 hexCharsSmall[] = "0123456789abcdef";
static u8 color = 0;
static u8 oldBG = 0, oldFG = 0;

static void printProcess(sProc *p) {
	tPid pid = getpid();
	debugf("(%d) Process:\n",pid);
	debugf("(%d) \tpid=%d\n",pid,p->pid);
	debugf("(%d) \tparentPid=%d\n",pid,p->parentPid);
	debugf("(%d) \tstate=%d\n",pid,p->state);
	debugf("(%d) \ttextPages=%d\n",pid,p->textPages);
	debugf("(%d) \tdataPages=%d\n",pid,p->dataPages);
	debugf("(%d) \tstackPages=%d\n",pid,p->stackPages);
}

typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

static void printProcess(sProc *p);

static void vid_printuSmall(u32 n,u8 base);

/**
 * Clears the screen
 */
void vid_clearScreen(void);

/**
 * Sets the background-color of the given line to the value
 *
 * @param line the line (0..n-1)
 * @param bg the background-color
 */
void vid_setLineBG(u8 line,eColor bg);

/**
 * Restores the color that has been saved by vid_useColor.
 */
void vid_restoreColor(void);

/**
 * @return the current foreground color
 */
eColor vid_getFGColor(void);

/**
 * @return the current background color
 */
eColor vid_getBGColor(void);

/**
 * Sets the foreground color to given value
 *
 * @param col the new color
 */
void vid_setFGColor(eColor col);

/**
 * Sets the background color to given value
 *
 * @param col the new color
 */
void vid_setBGColor(eColor col);

/**
 * Sets the given color and stores the current one. You may restore the previous state
 * by calling vid_restoreColor().
 *
 * @param bg your background-color
 * @param fg your foreground-color
 */
void vid_useColor(eColor bg,eColor fg);

/**
 * @return the line-number
 */
u8 vid_getLine(void);

/**
 * Walks to (lineEnd - pad), so that for example a string can be printed at the end of the line.
 * Note that this may overwrite existing characters!
 *
 * @param pad the number of characters to reach the line-end
 */
void vid_toLineEnd(u8 pad);

/**
 * Prints the given character to the current position on the screen
 *
 * @param c the character
 */
void vid_putchar(s8 c);

/**
 * Prints the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 */
void vid_printu(u32 n,u8 base);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
u8 vid_getuwidth(u32 n,u8 base);

/**
 * Prints the given string on the screen
 *
 * @param str the string
 */
void vid_puts(cstring str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
u8 vid_getswidth(cstring str);

/**
 * Prints the given signed 32-bit integer in base 10
 *
 * @param n the integer
 */
void vid_printn(s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
u8 vid_getnwidth(s32 n);

/**
 * The kernel-version of printf. Currently it supports:
 * %d: signed integer
 * %u: unsigned integer, base 10
 * %o: unsigned integer, base 8
 * %x: unsigned integer, base 16 (small letters)
 * %X: unsigned integer, base 16 (big letters)
 * %b: unsigned integer, base 2
 * %s: string
 * %c: character
 *
 * @param fmt the format
 */
void vid_printf(cstring fmt,...);

/**
 * Same as vid_printf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vid_vprintf(cstring fmt,va_list ap);

typedef struct {
	u8 id;
	u32 length;
} sConsoleMsg;

static sConsoleMsg *createMsg(u8 id,u32 length,void *buf) {
	sConsoleMsg *msg = (sConsoleMsg*)malloc(sizeof(sConsoleMsg) + length * sizeof(u8));
	msg->id = id;
	msg->length = length;
	if(length > 0)
		memcpy(msg + 1,buf,length);
	return msg;
}

static void freeMsg(sConsoleMsg *msg) {
	free(msg);
}

s32 main(void) {
#if 0
	s32 id = regService("console");
	if(id < 0)
		printLastError();
	else {
		if(fork() == 0) {
			s32 sfd = open("/system/services/console",IO_READ | IO_WRITE);
			if(sfd < 0)
				printLastError();
			else {
				s8 str[] = "Test";
				sConsoleMsg *msg = createMsg(ID_OUT,5,str);
				do {
					if(write(sfd,msg,sizeof(sConsoleMsg) + msg->length) < 0)
						printLastError();
					else {
						sendEOT(sfd);
					}
					yield();
				}
				while(1);
				freeMsg(msg);
			}
			close(sfd);
			while(1);
		}

		if(fork() == 0) {
			s32 sfd = open("/system/services/console",IO_READ | IO_WRITE);
			if(sfd < 0)
				printLastError();
			else {
				sConsoleMsg *msg = createMsg(ID_CLEAR,0,NULL);
				do {
					if(write(sfd,msg,sizeof(sConsoleMsg)) < 0)
						printLastError();
					else {
						sendEOT(sfd);
					}
					yield();
				}
				while(1);
				freeMsg(msg);
			}
			close(sfd);
			while(1);
		}

		videoBase = mapPhysical(0xB8000,COLS * ROWS * 2);
		video = videoBase;
		vid_clearScreen();
		vid_setFGColor(WHITE);
		vid_setBGColor(BLACK);

		static sConsoleMsg msg;
		do {
			s32 fd = waitForClient(id);
			if(fd < 0)
				printLastError();
			else {
				read(fd,&msg,sizeof(sConsoleMsg));
				if(msg.id == ID_OUT) {
					s8 *readBuf = malloc(msg.length * sizeof(s8));
					read(fd,readBuf,msg.length);
					debugf("Read: __%s__\n",readBuf);
					free(readBuf);
				}
				else if(msg.id == ID_IN) {

				}
				else if(msg.id == ID_CLEAR) {
					/*vid_clearScreen();*/
					debugf("CLEAR\n");
				}
				close(fd);
			}
		}
		while(1);

		unregService();
	}

#else

#if 0
	s32 id = regService("console");
	if(id < 0)
		printLastError();
	else {
		s8 buf[] = "Das ist ein Teststring\n";
		s32 fd = open("/system/services/console",IO_READ | IO_WRITE);
		if(fd < 0)
			printLastError();
		else {
			if(fork() == 0) {
				s8 *readBuf = malloc(strlen(buf) * sizeof(s8));
				while(1) {
					if(readBuf == NULL)
						printLastError();
					else {
						if(read(fd,readBuf,strlen(buf)) < 0)
							printLastError();
						else {
							sendEOT(fd);
							debugf("Read: %s\n",readBuf);
							/* reset buffer */
							*readBuf = '\0';
						}
					}
					yield();
				}
				close(fd);
			}
			else {
				while(1) {
					if(write(fd,buf,strlen(buf)) < 0)
						printLastError();
					else {
						sendEOT(fd);
						debugf("Wrote: %s\n",buf);
					}
					yield();
				}
			}
		}

		unregService();
	}
#endif


	s32 id = regService("console");
	if(id < 0)
		printLastError();
	else {
		debugf("Registered service\n");

		if(fork() == 0) {
			debugf("mypid=%d, ppid=%d\n",getpid(),getppid());
			s8 buf[100];
			s32 sfd = open("/system/services/console",IO_READ | IO_WRITE);
			debugf("Opened service with fd %d\n",sfd);
			yield();
			if(read(sfd,buf,100) < 0)
				printLastError();
			else
				debugf("Read: %s\n",buf);
			close(sfd);
			while(1);
		}

		yield();
		if(unregService(id) < 0)
			printLastError();
		else
			debugf("Unregistered service\n");
	}

#endif

	/*videoBase = mapPhysical(0xB8000,COLS * ROWS * 2);
	debugf("videoBase=0x%x\n",videoBase);
	if(videoBase == NULL) {
		printLastError();
	}
	else {
		video = videoBase;
		vid_clearScreen();
		vid_setFGColor(WHITE);
		vid_setBGColor(BLACK);

		vid_printf("Hallo, ich bins :P\n");
	}

	sSLList *l = sll_create();
	sll_append(l,1);
	sll_append(l,2);
	sll_append(l,3);

	sll_dbg_print(l);

	sll_destroy(l);*/

	/*test_register(&tModHeap);
	test_start();*/

	/*s32 count;

	count = changeSize(3);
	if(count < 0)
		printLastError();
	else
		debugf("dataPages = %d\n",count);

	count = changeSize(-3);
	if(count < 0)
		printLastError();
	else
		debugf("dataPages = %d\n",count);

	u32 *ptr[10],*p,*first;
	u32 c,i,x = 0;

	c = 10;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	first = ptr[x];
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 3;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 2;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 10;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	free(ptr[2]);
	free(ptr[0]);
	free(ptr[1]);

	/*for(i = 0;i < 25;i++) {
		debugf("0x%x = 0x%x\n",first,*first);
		first++;
	}

	ptr = (u32*)malloc(sizeof(u32) * 1025);
	debugf("ptr=0x%x\n",ptr);

	debugf("ptr=0x%x\n",malloc(4092));
	debugf("ptr=0x%x\n",malloc(3996));
	debugf("ptr=0x%x\n",malloc(10));

	printHeap();*/

	/*u16 pid = fork();
	if(pid == 0) {
		debugf("I am the child with pid %d\n",getpid());
	}
	else if(pid == -1) {
		debugf("Fork failed\n");
	}
	else {
		debugf("I am the parent, created child %d\n",pid);
	}

	while(true) {
		debugf("Hi, my pid is %d, parent is %d\n",getpid(),getppid());
	}*/

	/*if(regService("test") < 0)
		printLastError();
	else
		unregService();*/

	/*s32 dd;
	sDir *entry;
	debugf("Listing directory '/':\n");
	if((dd = opendir("/")) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			debugf("- 0x%x : %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}

	if(fork() == 0) {
		tFD fd;
		sProc proc;
		s8 path[] = "/system/processes/";
		s8 ppath[255];
		debugf("(%d) Listing all processes:\n",getpid());
		if((dd = opendir(path)) >= 0) {
			while((entry = readdir(dd)) != NULL) {
				strncpy(ppath,path,strlen(path));
				strncat(ppath,entry->name,strlen(entry->name));
				if((fd = open(ppath,IO_READ)) >= 0) {
					read(fd,&proc,sizeof(sProc));
					printProcess(&proc);
					close(fd);
				}
			}
			closedir(dd);
		}
		return 0;
	}*/

	while(1);
	return 0;
}

void vid_useColor(eColor bg,eColor fg) {
	oldBG = vid_getBGColor();
	oldFG = vid_getFGColor();
	vid_setBGColor(bg);
	vid_setFGColor(fg);
}

void vid_restoreColor(void) {
	vid_setBGColor(oldBG);
	vid_setFGColor(oldFG);
}

eColor vid_getFGColor(void) {
	return color & 0xF;
}

eColor vid_getBGColor(void) {
	return (color >> 4) & 0xF;
}

void vid_setFGColor(eColor col) {
	color = (color & 0xF0) | (col & 0xF);
}

void vid_setBGColor(eColor col) {
	color = (color & 0x0F) | ((col << 4) & 0xF0);
}

void vid_setLineBG(u8 line,eColor bg) {
	u8 col = ((bg << 4) & 0xF0) | color;
	u8 *addr = (u8*)(videoBase + line * COLS * 2);
	u8 *end = (u8*)((u32)addr + COLS * 2);
	for(addr++; addr < end; addr += 2) {
		*addr = col;
	}
}

u8 vid_getLine(void) {
	return ((u32)video - (u32)videoBase) / (COLS * 2);
}

void vid_toLineEnd(u8 pad) {
	u16 col = (u32)(video - videoBase) % (COLS * 2);
	video = (s8*)((u32)video + ((COLS * 2) - col) - pad * 2);
}

void vid_clearScreen(void) {
	u8 *addr = (u8*)videoBase;
	u8 *end = (u8*)((u32)addr + COLS * 2 * ROWS);
	for(; addr < end; addr++) {
		*addr = 0;
	}
}

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void) {
	u32 i;
	s8 *src,*dst;
	/* last line? */
	if(video >= (s8*)(videoBase + (ROWS - 1) * COLS * 2)) {
		/* copy all chars one line back */
		src = (s8*)(videoBase + COLS * 2);
		dst = (s8*)videoBase;
		for(i = 0; i < ROWS * COLS * 2; i++) {
			*dst++ = *src++;
		}
		/* to prev line */
		video -= COLS * 2;
	}
}

void vid_putchar(s8 c) {
	u32 i;
	vid_move();

	if(c == '\n') {
		/* to next line */
		video += COLS * 2;
		/* move cursor to line start */
		vid_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		video -= (u32)(video - videoBase) % (COLS * 2);
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vid_putchar(' ');
		}
	}
	else {
		*video = c;
		video++;
		*video = color;
		/* do an explicit newline if necessary */
		if(((u32)(video - videoBase) % (COLS * 2)) == COLS * 2 - 1)
			vid_putchar('\n');
		else
			video++;
	}
}

void vid_printu(u32 n,u8 base) {
	if(n >= base) {
		vid_printu(n / base,base);
	}
	vid_putchar(hexCharsBig[(n % base)]);
}

u8 vid_getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

void vid_puts(cstring str) {
	while(*str) {
		vid_putchar(*str++);
	}
}

u8 vid_getswidth(cstring str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
}

void vid_printn(s32 n) {
	if(n < 0) {
		vid_putchar('-');
		n = -n;
	}

	if(n >= 10) {
		vid_printn(n / 10);
	}
	vid_putchar('0' + n % 10);
}

u8 vid_getnwidth(s32 n) {
	/* we have at least one char */
	u8 width = 1;
	if(n < 0) {
		width++;
		n = -n;
	}
	while(n >= 10) {
		n /= 10;
		width++;
	}
	return width;
}

void vid_printf(cstring fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(cstring fmt,va_list ap) {
	s8 c,b,oldcolor = color,pad,padchar;
	string s;
	s32 n;
	u32 u;
	u8 width,base;

	while (1) {
		/* wait for a '%' */
		while ((c = *fmt++) != '%') {
			/* finished? */
			if (c == '\0') {
				return;
			}
			vid_putchar(c);
		}

		/* color given? */
		if(*fmt == ':') {
			color = ((*(++fmt) - '0') & 0xF) << 4;
			color |= (*(++fmt) - '0') & 0xF;
			fmt++;
		}

		/* read pad-character */
		pad = 0;
		if(*fmt == '0') {
			padchar = '0';
			fmt++;
		}
		else {
			padchar = ' ';
		}

		/* read pad-width */
		while(*fmt >= '0' && *fmt <= '9') {
			pad = pad * 10 + (*fmt - '0');
			fmt++;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
				n = va_arg(ap, s32);
				if(pad > 0) {
					width = vid_getnwidth(n);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				vid_printn(n);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				if(pad > 0) {
					width = vid_getuwidth(u,base);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				if(c == 'x')
					vid_printuSmall(u,base);
				else
					vid_printu(u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, string);
				if(pad > 0) {
					width = vid_getswidth(s);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				vid_puts(s);
				break;
			/* character */
			case 'c':
				b = (s8)va_arg(ap, u32);
				vid_putchar(b);
				break;
			/* all other */
			default:
				vid_putchar(c);
				break;
		}

		/* restore color */
		color = oldcolor;
	}
}

static void vid_printuSmall(u32 n,u8 base) {
	if(n >= base) {
		vid_printuSmall(n / base,base);
	}
	vid_putchar(hexCharsSmall[(n % base)]);
}
