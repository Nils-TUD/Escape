/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/irq.h>
#include <esc/thread.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <ipc/clientdevice.h>
#include <ipc/proto/input.h>
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

#define KEYBOARD_IRQ				1

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

static int kbIrqThread(void *arg);
static bool kb_waitOutBuf(void);
static bool kb_waitInBuf(void);

static ipc::ClientDevice<> *dev = NULL;

int main(void) {
	uint8_t kbdata;

	/* request io-ports */
	if(reqport(IOPORT_PIC) < 0)
		error("Unable to request io-port %d",IOPORT_PIC);
	if(reqport(IOPORT_KB_DATA) < 0)
		error("Unable to request io-port",IOPORT_KB_DATA);
	if(reqport(IOPORT_KB_CTRL) < 0)
		error("Unable to request io-port",IOPORT_KB_CTRL);

	/* wait for input buffer empty */
	if(!kb_waitInBuf())
		printe("Waiting for input buffer to empty timed out");

	/* first read all bytes from the buffer; maybe there are scancodes... */
	while(inbyte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL)
		inbyte(IOPORT_KB_DATA);

	print("Performing self test");

	/* self test */
	outbyte(IOPORT_KB_CTRL,0xAA);
	if(!kb_waitOutBuf())
		printe("Self test timed out");
	else {
		kbdata = inbyte(IOPORT_KB_DATA);
		if(kbdata != 0x55)
			printe("Keyboard-selftest failed: Got 0x%x, expected 0x55",kbdata);
	}

	print("Testing interface");

	/* test interface */
	outbyte(IOPORT_KB_CTRL,0xAB);
	if(!kb_waitOutBuf())
		printe("Interface test timed out");
	else {
		kbdata = inbyte(IOPORT_KB_DATA);
		if(kbdata != 0x00)
			printe("Interface-test failed: Got 0x%x, expected 0x00",kbdata);
	}

	/* write command byte */
	outbyte(IOPORT_KB_CTRL,0x60);
	if(!kb_waitInBuf())
		printe("Writing command byte 0x60 timed out");
	else {
		outbyte(IOPORT_KB_DATA,CMD_EN_OUTBUF_INTRPT | CMD_INHIBIT_OVERWRITE | CMD_IBM_COMPAT);
		if(!kb_waitInBuf())
			printe("Keyboard init timed out");
	}

#if 0
	/* reset */
	outbyte(IOPORT_KB_DATA,0xFF);
	sleep(20);
	kb_waitOutBuf();
	kbdata = inbyte(IOPORT_KB_DATA);
	/* send echo-command */
	outbyte(IOPORT_KB_DATA,0xEE);
	sleep(20);
	kb_waitOutBuf();
	kbdata = inbyte(IOPORT_KB_DATA);
	sleep(20);
	if(kbdata != 0xEE)
		error("Keyboard-echo failed: Got 0x%x, expected 0xEE",kbdata);
#endif

	print("Enabling keyboard");

	/* enable keyboard */
	outbyte(IOPORT_KB_DATA,0xF4);
	if(!kb_waitOutBuf())
		printe("Keyboard enable timed out");
	else {
		/* clear output buffer */
		kbdata = inbyte(IOPORT_KB_DATA);
	}

	/* disable LEDs
	kb_waitInBuf();
	outbyte(IOPORT_KB_DATA,0xED);
	kb_waitInBuf();
	outbyte(IOPORT_KB_DATA,0x0);*/

#if 0
	/* set scancode-set 1 */
	outbyte(IOPORT_KB_DATA,0xF0);
	outbyte(IOPORT_KB_DATA,0x1);
	sleep(20);
	kbdata = inbyte(IOPORT_KB_DATA);
	printf("Set scancode-set: %x\n",kbdata);
#endif

#if 0
	/* TODO doesn't work in qemu (but on real hardware). why? */
	/* set repeat-rate and delay */
	outbyte(IOPORT_KB_DATA,0xF3);
	outbyte(IOPORT_KB_DATA,0x24);	/*00100100*/
#endif

	/*outbyte(IOPORT_KB_DATA,0xF0);
	sleep(40);
	outbyte(IOPORT_KB_DATA,0x0);
	sleep(40);
	kbdata = inbyte(IOPORT_KB_DATA);
	printf("scancode-set=%x\n",kbdata);
	while(1);*/

	/* we want to get notified about keyboard interrupts */
	if(startthread(kbIrqThread,NULL) < 0)
		error("Unable to start IRQ-thread");

	dev = new ipc::ClientDevice<>("/dev/keyb",0110,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CLOSE);
	dev->loop();

	/* clean up */
	relport(IOPORT_PIC);
	relport(IOPORT_KB_DATA);
	relport(IOPORT_KB_CTRL);
	return EXIT_SUCCESS;
}

static int kbIrqThread(A_UNUSED void *arg) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	int sem = semcrtirq(KEYBOARD_IRQ,"PS/2 Keyboard");
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",KEYBOARD_IRQ);
	while(1) {
		semdown(sem);

		if(!(inbyte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL))
			continue;

		uint8_t sc = inbyte(IOPORT_KB_DATA);
		ipc::Keyb::Event ev;
		if(kb_set1_getKeycode(&ev.isBreak,&ev.keycode,sc)) {
			ipc::IPCBuf ib(buffer,sizeof(buffer));
			ib << ev;
			try {
				dev->broadcast(ipc::Keyb::Event::MID,ib);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
		}
	}
	return 0;
}

static bool kb_waitOutBuf(void) {
	time_t elapsed = 0;
	uint8_t status;
	do {
		status = inbyte(IOPORT_KB_CTRL);
		if((status & STATUS_OUTBUF_FULL) == 0) {
			sleep(SLEEP_TIME);
			elapsed += SLEEP_TIME;
		}
	}
	while((status & STATUS_OUTBUF_FULL) == 0 && elapsed < TIMEOUT);
	return (status & STATUS_OUTBUF_FULL) != 0;
}

static bool kb_waitInBuf(void) {
	time_t elapsed = 0;
	uint8_t status;
	do {
		status = inbyte(IOPORT_KB_CTRL);
		if((status & STATUS_INBUF_FULL) != 0) {
			sleep(SLEEP_TIME);
			elapsed += SLEEP_TIME;
		}
	}
	while((status & STATUS_INBUF_FULL) != 0 && elapsed < TIMEOUT);
	return (status & STATUS_INBUF_FULL) == 0;
}
