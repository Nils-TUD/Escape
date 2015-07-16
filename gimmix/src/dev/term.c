/**
 * $Id: term.c 218 2011-06-04 15:46:40Z nasmussen $
 */

#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "common.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "exception.h"
#include "dev/term.h"
#include "dev/timer.h"
#include "mmix/mem.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "sim.h"
#include "config.h"

#define TERM_XPOS			1600	// x offset of all windows
#define TERM_YPOS			0		// y offset of all windows

#define TERM_RCVR_CTRL		0		// receiver control register
#define TERM_RCVR_DATA		8		// receiver data register
#define TERM_XMTR_CTRL		16		// transmitter control register
#define TERM_XMTR_DATA		24		// transmitter data register

#define TERM_RCVR_RDY		0x01	// receiver has a character
#define TERM_RCVR_IEN		0x02	// enable receiver interrupt

#define TERM_XMTR_RDY		0x01	// transmitter accepts a character
#define TERM_XMTR_IEN		0x02	// enable transmitter interrupt

typedef struct {
	pid_t pid;
	FILE *in;
	FILE *out;
	octa rcvrCtrl;
	octa rcvrData;
	int rcvrIRQ;
	octa xmtrCtrl;
	octa xmtrData;
	int xmtrIRQ;
} sTerminal;

static void term_reset(void);
static octa term_read(octa addr,bool sideEffects);
static void term_write(octa addr,octa data);
static void term_shutdown(void);
static void term_rcvrCallback(int dev);
static void term_xmtrCallback(int dev);
static int term_openPty(int *master,int *slave,char *name);
static void term_makeRaw(struct termios *tp);

static sDevice termDevs[TERM_MAX];
static sTerminal terminals[TERM_MAX];
static int numTerminals;

void term_init(void) {
	int master = -1,slave = -1;
	char ptyName[100];
	char ptyTitle[100];
	struct termios termios;

	numTerminals = cfg_getTermCount();
	for(int i = 0; i < numTerminals; i++) {
		// register device
		int len = sprintf(ptyName,"Terminal %d",i);
		termDevs[i].name = mem_alloc(len + 1);
		strcpy((char*)termDevs[i].name,ptyName);
		termDevs[i].startAddr = TERM_START_ADDR + i * DEV_REG_SPACE;
		// 4 registers
		termDevs[i].endAddr = TERM_START_ADDR + i * DEV_REG_SPACE + 4 * sizeof(octa) - 1;
		termDevs[i].irqMask = ((octa)1 << (TERM_START_IRQ + i * 2)) |
				((octa)1 << (TERM_START_IRQ + i * 2 + 1));
		termDevs[i].reset = term_reset;
		termDevs[i].read = term_read;
		termDevs[i].write = term_write;
		termDevs[i].shutdown = term_shutdown;
		bus_register(termDevs + i);

		// open pseudo terminal
		if(term_openPty(&master,&slave,ptyName) < 0)
			sim_error("cannot open pseudo terminal %d",i);
		// set mode to raw
		tcgetattr(slave,&termios);
		term_makeRaw(&termios);
		tcsetattr(slave,TCSANOW,&termios);
		// fork and exec a new xterm
		terminals[i].pid = fork();
		if(terminals[i].pid < 0)
			sim_error("cannot fork xterm process %d",i);
		if(terminals[i].pid == 0) {
			char geo[20];
			// terminal process
			setpgid(0,0);
			close(2);
			close(master);
			// it's annoying to have to adjust the window-position every time gimmix starts :)
			sprintf(geo, "+%d+%d", TERM_XPOS + (i / 2) * 500,TERM_YPOS + (i % 2) * 400);
			sprintf(ptyName,"-Sab%d",slave);
			sprintf(ptyTitle,"GIMMIX Terminal %d",i);
			execlp("xterm","xterm","-geo",geo,"-title",ptyTitle,ptyName,NULL);
			sim_error("cannot exec xterm process %d",i);
		}
		terminals[i].in = fdopen(master,"r");
		setvbuf(terminals[i].in,NULL,_IONBF,0);
		terminals[i].out = fdopen(master,"w");
		setvbuf(terminals[i].out,NULL,_IONBF,0);
		// skip the window id written by xterm
		while(fgetc(terminals[i].in) != '\n')
			;
	}
	term_reset();
}

static void term_reset(void) {
	for(int i = 0; i < numTerminals; i++) {
		terminals[i].rcvrCtrl = 0;
		terminals[i].rcvrData = 0;
		terminals[i].rcvrIRQ = TERM_START_IRQ + i * 2 + 1;
		timer_start(TERM_RCVR_MSEC,termDevs + i,term_rcvrCallback,i);
		terminals[i].xmtrCtrl = TERM_XMTR_RDY;
		terminals[i].xmtrData = 0;
		terminals[i].xmtrIRQ = TERM_START_IRQ + i * 2;
	}
}

static void term_shutdown(void) {
	// kill and wait for all xterm processes
	for(int i = 0; i < numTerminals; i++) {
		if(terminals[i].pid > 0) {
			kill(terminals[i].pid,SIGKILL);
			waitpid(terminals[i].pid,NULL,0);
		}
	}
}

static octa term_read(octa addr,bool sideEffects) {
	int dev = DEV_NO(addr);
	// illegal device
	if(dev >= numTerminals)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	octa data = 0;
	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case TERM_RCVR_CTRL:
			data = terminals[dev].rcvrCtrl;
			break;

		case TERM_RCVR_DATA:
			if(sideEffects) {
				terminals[dev].rcvrCtrl &= ~TERM_RCVR_RDY;
				// lower terminal rcvr interrupt
				if(terminals[dev].rcvrCtrl & TERM_RCVR_IEN)
					cpu_resetInterrupt(terminals[dev].rcvrIRQ);
			}
			data = terminals[dev].rcvrData;
			break;

		case TERM_XMTR_CTRL:
			data = terminals[dev].xmtrCtrl;
			break;

		case TERM_XMTR_DATA:
			// this register is write-only
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;

		default:
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
	return data;
}

static void term_write(octa addr,octa data) {
	int dev = DEV_NO(addr);
	// illegal device
	if(dev >= numTerminals)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case TERM_RCVR_CTRL:
			if(data & TERM_RCVR_IEN)
				terminals[dev].rcvrCtrl |= TERM_RCVR_IEN;
			else
				terminals[dev].rcvrCtrl &= ~TERM_RCVR_IEN;
			if(data & TERM_RCVR_RDY)
				terminals[dev].rcvrCtrl |= TERM_RCVR_RDY;
			else
				terminals[dev].rcvrCtrl &= ~TERM_RCVR_RDY;

			// raise/lower terminal rcvr interrupt
			if((terminals[dev].rcvrCtrl & TERM_RCVR_IEN) != 0 &&
				(terminals[dev].rcvrCtrl & TERM_RCVR_RDY) != 0) {
				cpu_setInterrupt(terminals[dev].rcvrIRQ);
			}
			else
				cpu_resetInterrupt(terminals[dev].rcvrIRQ);
			break;

		case TERM_RCVR_DATA:
			// this register is read-only
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;

		case TERM_XMTR_CTRL:
			if(data & TERM_XMTR_IEN)
				terminals[dev].xmtrCtrl |= TERM_XMTR_IEN;
			else
				terminals[dev].xmtrCtrl &= ~TERM_XMTR_IEN;
			if(data & TERM_XMTR_RDY)
				terminals[dev].xmtrCtrl |= TERM_XMTR_RDY;
			else
				terminals[dev].xmtrCtrl &= ~TERM_XMTR_RDY;

			// raise/lower terminal xmtr interrupt
			if((terminals[dev].xmtrCtrl & TERM_XMTR_IEN) != 0 &&
				(terminals[dev].xmtrCtrl & TERM_XMTR_RDY) != 0) {
				cpu_setInterrupt(terminals[dev].xmtrIRQ);
			}
			else
				cpu_resetInterrupt(terminals[dev].xmtrIRQ);
			break;

		case TERM_XMTR_DATA:
			terminals[dev].xmtrData = data & 0xFF;
			terminals[dev].xmtrCtrl &= ~TERM_XMTR_RDY;
			// lower terminal xmtr interrupt
			if(terminals[dev].xmtrCtrl & TERM_XMTR_IEN)
				cpu_resetInterrupt(terminals[dev].xmtrIRQ);
			timer_start(TERM_XMTR_MSEC,termDevs + dev,term_xmtrCallback,dev);
			break;

		default:
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
}

static void term_rcvrCallback(int dev) {
	timer_start(TERM_RCVR_MSEC,termDevs + dev,term_rcvrCallback,dev);
	int c = fgetc(terminals[dev].in);
	// no character typed?
	if(c == EOF)
		return;

	// any character typed
	terminals[dev].rcvrData = c & 0xFF;
	terminals[dev].rcvrCtrl |= TERM_RCVR_RDY;
	// raise terminal rcvr interrupt
	if(terminals[dev].rcvrCtrl & TERM_RCVR_IEN)
		cpu_setInterrupt(terminals[dev].rcvrIRQ);
}

static void term_xmtrCallback(int dev) {
	fputc(terminals[dev].xmtrData & 0xFF,terminals[dev].out);
	terminals[dev].xmtrCtrl |= TERM_XMTR_RDY;
	// raise terminal xmtr interrupt
	if(terminals[dev].xmtrCtrl & TERM_XMTR_IEN)
		cpu_setInterrupt(terminals[dev].xmtrIRQ);
}

static int term_openPty(int *master,int *slave,char *name) {
	// try to open master
	strcpy(name,"/dev/ptmx");
	*master = open(name,O_RDWR | O_NONBLOCK);
	// open failed?
	if(*master < 0)
		return -1;
	grantpt(*master);
	unlockpt(*master);
	// master opened, try to open slave
	strcpy(name,ptsname(*master));
	*slave = open(name,O_RDWR | O_NONBLOCK);
	if(*slave < 0) {
		// open failed, close master
		close(*master);
		return -1;
	}
	// all is well
	return 0;
}

static void term_makeRaw(struct termios *tp) {
	tp->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tp->c_oflag &= ~OPOST;
	tp->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tp->c_cflag &= ~(CSIZE | PARENB);
	tp->c_cflag |= CS8;
}
