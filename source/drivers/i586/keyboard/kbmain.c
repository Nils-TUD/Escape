/**
 * $Id: kbmain.c 1001 2011-07-30 18:56:36Z nasmussen $
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/arch/i586/vm86.h>
#include <esc/driver/vterm.h>
#include <esc/driver/video.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/ringbuffer.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "set1.h"

/* io-ports */
#define IOPORT_PIC					0x20
#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64

#define KBC_CMD_DISABLE_MOUSE		0xA7
#define KBC_CMD_ENABLE_MOUSE		0xA8

/* indicates that the output-buffer is full */
#define STATUS_OUTBUF_FULL			(1 << 0)
/* indicates that the input-buffer is full */
#define STATUS_INBUF_FULL			(1 << 1)
/* data for mouse */
#define STATUS_MOUSE_DATA_AVAIL		(1 << 5)

/* converts the scan-code to the ones IBM PCs expect */
#define CMD_IBM_COMPAT				(1 << 6)
/* performs no scan-code-conversion and no partity-check */
#define CMD_IBM_MODE				(1 << 5)
#define CMD_DISABLE_KEYBOARD		(1 << 4)
#define CMD_INHIBIT_OVERWRITE		(1 << 3)
/* bit-value is written to the system-flag in controller-status-register */
#define CMD_SYSTEM					(1 << 2)
/* raises an interrupt when the output-buffer is full */
#define CMD_EN_OUTBUF_INTRPT		(1 << 0)

/* ICW = initialisation command word */
#define PIC_ICW1					0x20

#define KB_MOUSE_LOCK				0x1

#define BUF_SIZE					128
#define SC_BUF_SIZE					16
#define TIMEOUT						60
#define SLEEP_TIME					20

static void kbStartDbgConsole(void);
static void kbIntrptHandler(int sig);
static void kb_waitOutBuf(void);
static void kb_waitInBuf(void);

/* file-descriptor for ourself */
static sMsg msg;
static sRingBuf *rbuf;
static uint8_t scBuf[SC_BUF_SIZE];
static size_t scReadPos = 0;
static size_t scWritePos = 0;

int main(void) {
	int id;
	msgid_t mid;
	uint8_t kbdata;

	/* create buffers */
	rbuf = rb_create(sizeof(sKbData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* request io-ports */
	if(requestIOPort(IOPORT_PIC) < 0)
		error("Unable to request io-port %d",IOPORT_PIC);
	if(requestIOPort(IOPORT_KB_DATA) < 0)
		error("Unable to request io-port",IOPORT_KB_DATA);
	if(requestIOPort(IOPORT_KB_CTRL) < 0)
		error("Unable to request io-port",IOPORT_KB_CTRL);

	/* wait for input buffer empty */
	kb_waitInBuf();

	/* first read all bytes from the buffer; maybe there are scancodes... */
	while(inByte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL)
		inByte(IOPORT_KB_DATA);

	/* self test */
	outByte(IOPORT_KB_CTRL,0xAA);
	kb_waitOutBuf();
	kbdata = inByte(IOPORT_KB_DATA);
	if(kbdata != 0x55)
		error("Keyboard-selftest failed: Got 0x%x, expected 0x55",kbdata);

	/* test interface */
	outByte(IOPORT_KB_CTRL,0xAB);
	kb_waitOutBuf();
	kbdata = inByte(IOPORT_KB_DATA);
	if(kbdata != 0x00)
		error("Interface-test failed: Got 0x%x, expected 0x00",kbdata);

	/* write command byte */
	outByte(IOPORT_KB_CTRL,0x60);
	kb_waitInBuf();
	outByte(IOPORT_KB_DATA,CMD_EN_OUTBUF_INTRPT | CMD_INHIBIT_OVERWRITE | CMD_IBM_COMPAT);
	kb_waitInBuf();

#if 0
	/* reset */
	outByte(IOPORT_KB_DATA,0xFF);
	sleep(20);
	kb_waitOutBuf();
	kbdata = inByte(IOPORT_KB_DATA);
#endif
	/* send echo-command */
	outByte(IOPORT_KB_DATA,0xEE);
	sleep(20);
	kb_waitOutBuf();
	kbdata = inByte(IOPORT_KB_DATA);
	sleep(20);
	if(kbdata != 0xEE)
		error("Keyboard-echo failed: Got 0x%x, expected 0xEE",kbdata);

	/* enable keyboard */
	outByte(IOPORT_KB_DATA,0xF4);
	kb_waitOutBuf();
	/* clear output buffer */
	kbdata = inByte(IOPORT_KB_DATA);

	/* disable LEDs
	kb_waitInBuf();
	outByte(IOPORT_KB_DATA,0xED);
	kb_waitInBuf();
	outByte(IOPORT_KB_DATA,0x0);*/

#if 0
	/* set scancode-set 1 */
	outByte(IOPORT_KB_DATA,0xF0);
	outByte(IOPORT_KB_DATA,0x1);
	sleep(20);
	kbdata = inByte(IOPORT_KB_DATA);
	printf("Set scancode-set: %x\n",kbdata);
#endif

#if 0
	/* TODO doesn't work in qemu (but on real hardware). why? */
	/* set repeat-rate and delay */
	outByte(IOPORT_KB_DATA,0xF3);
	outByte(IOPORT_KB_DATA,0x24);	/*00100100*/
#endif

	/*outByte(IOPORT_KB_DATA,0xF0);
	sleep(40);
	outByte(IOPORT_KB_DATA,0x0);
	sleep(40);
	kbdata = inByte(IOPORT_KB_DATA);
	printf("scancode-set=%x\n",kbdata);
	while(1);*/

	/* we want to get notified about keyboard interrupts */
	if(setSigHandler(SIG_INTRPT_KB,kbIntrptHandler) < 0)
		error("Unable to announce sig-handler for %d",SIG_INTRPT_KB);

	id = createdev("/dev/keyboard",DEV_TYPE_CHAR,DEV_READ);
	if(id < 0)
		error("Unable to register device 'keyboard'");

    /* wait for commands */
	while(1) {
		int fd;

		/* translate scancodes to keycodes */
		if(scReadPos != scWritePos) {
			while(scReadPos != scWritePos) {
				sKbData data;
				if(kb_set1_getKeycode(&data.isBreak,&data.keycode,scBuf[scReadPos])) {
					/* F12 starts the kernel-debugging-console */
					if(!data.isBreak && data.keycode == VK_F12)
						kbStartDbgConsole();
					else
						rb_write(rbuf,&data);
				}
				scReadPos = (scReadPos + 1) % SC_BUF_SIZE;
			}
			if(rb_length(rbuf) > 0)
				fcntl(id,F_SETDATA,true);
		}

		fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("[KB] Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					/* offset is ignored here */
					size_t count = msg.args.arg2 / sizeof(sKbData);
					sKbData *buffer = (sKbData*)malloc(count * sizeof(sKbData));
					msg.args.arg1 = 0;
					if(buffer)
						msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sKbData);
					msg.args.arg2 = rb_length(rbuf) > 0;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(buffer) {
						send(fd,MSG_DEV_READ_RESP,buffer,msg.args.arg1);
						free(buffer);
					}
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_DATA);
	releaseIOPort(IOPORT_KB_CTRL);
	rb_destroy(rbuf);
	close(id);

	return EXIT_SUCCESS;
}

static void kbStartDbgConsole(void) {
	/* switch to vga-text-mode */
	int fd = open("/dev/video",IO_MSGS);
	if(fd >= 0) {
		video_setMode(fd);
		close(fd);
	}

	/* start debugger */
	debug();

	/* restore video-mode */
	/* TODO this is not perfect since it causes problems when we're in GUI-mode.
	 * But its for debugging, so its ok, I think :) */
	fd = open("/dev/vterm0",IO_MSGS);
	if(fd >= 0) {
		vterm_setEnabled(fd,true);
		close(fd);
	}
}

static void kbIntrptHandler(A_UNUSED int sig) {
	if(!(inByte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL))
		return;

	scBuf[scWritePos] = inByte(IOPORT_KB_DATA);
	scWritePos = (scWritePos + 1) % SC_BUF_SIZE;
}

static void kb_waitOutBuf(void) {
	time_t time = 0;
	uint8_t status;
	do {
		status = inByte(IOPORT_KB_CTRL);
		if((status & STATUS_OUTBUF_FULL) == 0) {
			sleep(SLEEP_TIME);
			time += SLEEP_TIME;
		}
	}
	while((status & STATUS_OUTBUF_FULL) == 0 && time < TIMEOUT);
}

static void kb_waitInBuf(void) {
	time_t time = 0;
	uint8_t status;
	do {
		status = inByte(IOPORT_KB_CTRL);
		if((status & STATUS_INBUF_FULL) != 0) {
			sleep(SLEEP_TIME);
			time += SLEEP_TIME;
		}
	}
	while((status & STATUS_INBUF_FULL) != 0 && time < TIMEOUT);
}
