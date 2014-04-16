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
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/driver.h>
#include <esc/ringbuffer.h>
#include <esc/messages.h>
#include <esc/irq.h>
#include <ipc/clientdevice.h>
#include <ipc/proto/input.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

/* io ports */
#define IOPORT_KB_CTRL				0x64
#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_INOUTBUF			0x60

#define MOUSE_IRQ					12

#define KBC_CMD_READ_STATUS			0x20
#define KBC_CMD_SET_STATUS			0x60
#define KBC_CMD_DISABLE_MOUSE		0xA7
#define KBC_CMD_ENABLE_MOUSE		0xA8
#define KBC_CMD_DISABLE_KEYBOARD	0xAD
#define KBC_CMD_ENABLE_KEYBOARD		0xAE
#define KBC_CMD_NEXT2MOUSE			0xD4
#define MOUSE_CMD_SETSAMPLE			0xF3
#define MOUSE_CMD_STREAMING			0xF4
#define MOUSE_CMD_GETDEVID			0xF2

/* bits of the status-byte */
#define KBC_STATUS_DATA_AVAIL		(1 << 0)
#define KBC_STATUS_BUSY				(1 << 1)
#define KBC_STATUS_SELFTEST_OK		(1 << 2)
#define KBC_STATUS_LAST_WAS_CMD		(1 << 3)
#define KBC_STATUS_KB_LOCKED		(1 << 4)
#define KBC_STATUS_MOUSE_DATA_AVAIL	(1 << 5)
#define KBC_STATUS_TIMEOUT			(1 << 6)
#define KBC_STATUS_PARITY_ERROR		(1 << 7)

/* some bits in the command-byte */
#define KBC_CMDBYTE_TRANSPSAUX		(1 << 6)
#define KBC_CMDBYTE_DISABLE_KB		(1 << 4)
#define KBC_CMDBYTE_ENABLE_IRQ12	(1 << 1)
#define KBC_CMDBYTE_ENABLE_IRQ1		(1 << 0)

#define KB_MOUSE_LOCK				0x1

#define INPUT_BUF_SIZE				128
#define PACK_BUF_SIZE				32

static int irqThread(void *arg);
static void kb_init(void);
static void kb_checkCmd(void);
static uint16_t kb_read(void);
static uint8_t kb_writeMouse(uint8_t cmd);

typedef union {
	uchar	yOverflow : 1,
		xOverflow : 1,
		ySign : 1,
		xSign : 1,
		: 1,
		middleBtn : 1,
		rightBtn : 1,
		leftBtn : 1;
	uchar all;
} uStatus;

static uchar byteNo = 0;
static int sid;
static bool wheel = false;
static ipc::ClientDevice<> *dev = NULL;

int main(void) {
	/* request io-ports */
	if(reqport(IOPORT_KB_CTRL) < 0 || reqport(IOPORT_KB_DATA) < 0)
		error("Unable to request io-ports");

	kb_init();

	/* reg intrpt-handler */
	if(startthread(irqThread,NULL) < 0)
		error("Unable to start irq-thread");

	dev = new ipc::ClientDevice<>("/dev/mouse",0110,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CLOSE);
	dev->loop();

	/* cleanup */
	relport(IOPORT_KB_CTRL);
	relport(IOPORT_KB_DATA);
	close(sid);
	return EXIT_SUCCESS;
}

static int irqThread(A_UNUSED void *arg) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::Mouse::Event ev;

	int sem = semcrtirq(MOUSE_IRQ,"PS/2 Mouse");
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",MOUSE_IRQ);
	while(1) {
		semdown(sem);

		/* check if there is mouse-data */
		uint8_t status = inbyte(IOPORT_KB_CTRL);
		if(!(status & KBC_STATUS_MOUSE_DATA_AVAIL))
			continue;

		switch(byteNo) {
			case 0: {
				uStatus st;
				st.all = inbyte(IOPORT_KB_DATA);
				ev.buttons = (st.leftBtn << 2) | (st.rightBtn << 1) | (st.middleBtn << 0);
				byteNo++;
			}
			break;
			case 1:
				ev.x = (char)inbyte(IOPORT_KB_DATA);
				byteNo++;
				break;
			case 2:
				ev.y = (char)inbyte(IOPORT_KB_DATA);
				if(wheel)
					byteNo++;
				else {
					byteNo = 0;
					ev.z = 0;
				}
				break;
			case 3:
				ev.z = (char)inbyte(IOPORT_KB_DATA);
				byteNo = 0;
				break;
		}

		if(byteNo == 0) {
			ipc::IPCBuf ib(buffer,sizeof(buffer));
			ib << ev;
			try {
				dev->broadcast(ipc::Mouse::Event::MID,ib);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
		}
	}
}

static void kb_init(void) {
	uint8_t id,cmdByte;

	print("Enabling mouse");
	outbyte(IOPORT_KB_CTRL,KBC_CMD_ENABLE_MOUSE);
	kb_checkCmd();

	print("Putting mouse in streaming mode");
	kb_writeMouse(MOUSE_CMD_STREAMING);

	/* read cmd byte */
	outbyte(IOPORT_KB_CTRL,KBC_CMD_READ_STATUS);
	kb_checkCmd();
	cmdByte = kb_read();
	outbyte(IOPORT_KB_CTRL,KBC_CMD_SET_STATUS);
	kb_checkCmd();

	/* somehow cmdByte is 0 in vbox and qemu. we have to enable TRANSPSAUX in this case.
	 * otherwise we get strange scancodes in the keyboard-driver */
	print("Enabling IRQs");
	cmdByte |= KBC_CMDBYTE_TRANSPSAUX | KBC_CMDBYTE_ENABLE_IRQ12 | KBC_CMDBYTE_ENABLE_IRQ1;
	cmdByte &= ~KBC_CMDBYTE_DISABLE_KB;
	outbyte(IOPORT_KB_DATA,cmdByte);
	kb_checkCmd();

	print("Checking whether we have a wheel");
	/* enable mouse-wheel by setting sample-rate to 200, 100 and 80 and reading the device-id */
	kb_writeMouse(MOUSE_CMD_SETSAMPLE);
	kb_writeMouse(200);
	kb_writeMouse(MOUSE_CMD_SETSAMPLE);
	kb_writeMouse(100);
	kb_writeMouse(MOUSE_CMD_SETSAMPLE);
	kb_writeMouse(80);
	kb_writeMouse(MOUSE_CMD_GETDEVID);
	id = kb_read();
	if(id == 3 || id == 4) {
		print("Detected mouse-wheel");
		wheel = true;
	}
}

static void kb_checkCmd(void) {
	while(inbyte(IOPORT_KB_CTRL) & KBC_STATUS_BUSY)
		;
}

static uint16_t kb_read(void) {
	uint16_t c = 0;
	uint8_t status;
	while(c++ < 0xFFFF && !((status = inbyte(IOPORT_KB_CTRL)) & KBC_STATUS_DATA_AVAIL));
	if(!(status & KBC_STATUS_DATA_AVAIL))
		return 0xFF00;
	return inbyte(IOPORT_KB_DATA);
}

static uint8_t kb_writeMouse(uint8_t cmd) {
	outbyte(IOPORT_KB_CTRL,KBC_CMD_NEXT2MOUSE);
	kb_checkCmd();
	outbyte(IOPORT_KB_DATA,cmd);
	kb_checkCmd();
	return kb_read();
}
