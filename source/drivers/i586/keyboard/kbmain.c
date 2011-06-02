/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <arch/i586/ports.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/ringbuffer.h>
#include <esc/vm86.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errors.h>

#include "set1.h"

#define BUF_SIZE					128
#define TIMEOUT						60
#define SLEEP_TIME					20

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

/**
 * The keyboard-interrupt handler
 */
static void kbIntrptHandler(int sig);
static void kb_waitOutBuf(void);
static void kb_waitInBuf(void);

/* file-descriptor for ourself */
static sMsg msg;
static sRingBuf *rbuf;
static sRingBuf *ibuf;
static bool moving = false;

int main(void) {
	tFD id;
	tMsgId mid;
	uint8_t kbdata;

	/* create buffers */
	rbuf = rb_create(sizeof(sKbData),BUF_SIZE,RB_OVERWRITE);
	ibuf = rb_create(sizeof(sKbData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL || ibuf == NULL)
		error("Unable to create the ring-buffers");

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

	id = regDriver("keyboard",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'keyboard'");

    /* wait for commands */
	while(1) {
		tFD fd;

		/* move keycodes (we can't access ibuf while doing this) */
		moving = true;
		rb_move(rbuf,ibuf,rb_length(ibuf));
		if(rb_length(rbuf) > 0)
			fcntl(id,F_SETDATA,true);
		moving = false;

		fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED)
				printe("[KB] Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DRV_READ: {
					/* offset is ignored here */
					size_t count = msg.args.arg2 / sizeof(sKbData);
					sKbData *buffer = (sKbData*)malloc(count * sizeof(sKbData));
					msg.args.arg1 = 0;
					if(buffer)
						msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sKbData);
					msg.args.arg2 = rb_length(rbuf) > 0;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(buffer) {
						send(fd,MSG_DRV_READ_RESP,buffer,count * sizeof(sKbData));
						free(buffer);
					}
				}
				break;
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_DATA);
	releaseIOPort(IOPORT_KB_CTRL);
	rb_destroy(ibuf);
	rb_destroy(rbuf);
	close(id);

	return EXIT_SUCCESS;
}

static void kbIntrptHandler(int sig) {
	uint8_t scanCode;
	sKbData data;
	UNUSED(sig);

	if(!(inByte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL))
		return;
	scanCode = inByte(IOPORT_KB_DATA);
	/* if we're currently moving stuff from ibuf to rbuf, we can't access ibuf */
	/* so, simply skip the scancode in this case */
	if(!moving) {
		if(kb_set1_getKeycode(&data.isBreak,&data.keycode,scanCode)) {
#if DEBUGGING
			/* F12 starts the kernel-debugging-console */
			if(!data.isBreak && data.keycode == VK_F12) {
				/* switch to vga-text-mode */
				sVM86Regs vmregs;
				memclear(&vmregs,sizeof(vmregs));
				vmregs.ax = 0x2;
				vm86int(0x10,&vmregs,NULL,0);

				/* start debugger */
				debug();

				/* restore video-mode */
				/* TODO this is not perfect since it causes problems when we're in GUI-mode.
				 * But its for debugging, so its ok, I think :) */
				tFD fd = open("/dev/vterm0",IO_WRITE);
				if(fd >= 0) {
					sendRecvMsgData(fd,MSG_VT_ENABLE,NULL,0);
					close(fd);
				}
				return;
			}
#endif
			/* write in buffer */
			rb_write(ibuf,&data);
		}
	}
	/* ack scancode
	outByte(IOPORT_PIC,PIC_ICW1);*/
}

static void kb_waitOutBuf(void) {
	tTime time = 0;
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
	tTime time = 0;
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
