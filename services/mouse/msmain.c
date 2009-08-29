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
#include <esc/ports.h>
#include <esc/io.h>
#include <esc/signals.h>
#include <esc/service.h>
#include <esc/fileio.h>
#include <messages.h>
#include <esc/lock.h>
#include <stdlib.h>
#include <ringbuffer.h>
#include <errors.h>

/* FIXME: THIS MAY CAUSE TROUBLE SINCE WE'RE USING AN IO-PORT THAT IS USED BY THE
 * KEYBOARD-SERVICE, TOO */
/* we need some kind of locking-mechanism here to fix it.. */

/* io ports */
#define IOPORT_KB_CTRL				0x64
#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_INOUTBUF			0x60

#define KBC_CMD_READ_STATUS			0x20
#define KBC_CMD_SET_STATUS			0x60
#define KBC_CMD_DISABLE_MOUSE		0xA7
#define KBC_CMD_ENABLE_MOUSE		0xA8
#define KBC_CMD_DISABLE_KEYBOARD	0xAD
#define KBC_CMD_ENABLE_KEYBOARD		0xAE
#define KBC_CMD_NEXT2MOUSE			0xD4
#define MOUSE_CMD_STREAMING			0xF4

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
#define KBC_CMDBYTE_DISABLE_KB		(1 << 4)
#define KBC_CMDBYTE_ENABLE_IRQ12	(1 << 1)
#define KBC_CMDBYTE_ENABLE_IRQ1		(1 << 0)

#define KB_MOUSE_LOCK				0x1

#define INPUT_BUF_SIZE				128

static void irqHandler(tSig sig,u32 data);
static void kb_init(void);
static void kb_checkCmd(void);
static u16 kb_read(void);
static u16 kb_readMouse(void);
static u16 kb_writeMouse(u8 cmd);

/* a mouse-packet */
typedef struct {
	union {
		u8	yOverflow : 1,
			xOverflow : 1,
			ySign : 1,
			xSign : 1,
			: 1,
			middleBtn : 1,
			rightBtn : 1,
			leftBtn : 1;
		u8 all;
	} status;
	s8 xcoord;
	s8 ycoord;
} sMousePacket;

static u8 byteNo = 0;
static tServ sid;
static sMsg msg;
static sRingBuf *ibuf;
static sRingBuf *rbuf;
static sMouseData mdata;
static sMousePacket packet;

int main(void) {
	tMsgId mid;
	tServ client;

	/* request io-ports */
	if(requestIOPort(IOPORT_KB_CTRL) < 0 || requestIOPort(IOPORT_KB_DATA) < 0) {
		printe("Unable to request io-ports");
		return EXIT_FAILURE;
	}

	kb_init();

	/* reg intrpt-handler */
	if(setSigHandler(SIG_INTRPT_MOUSE,irqHandler) < 0) {
		printe("Unable to announce interrupt-handler");
		return EXIT_FAILURE;
	}

	/* reg service and open ourself */
	sid = regService("mouse",SERV_DRIVER);
	if(sid < 0) {
		printe("Unable to register service '%s'","mouse");
		return EXIT_FAILURE;
	}

	/* create input-buffer */
	ibuf = rb_create(sizeof(sMouseData),INPUT_BUF_SIZE,RB_OVERWRITE);
	rbuf = rb_create(sizeof(sMouseData),INPUT_BUF_SIZE,RB_OVERWRITE);
	if(ibuf == NULL || rbuf == NULL) {
		printe("Unable to create ring-buffers");
		return EXIT_FAILURE;
	}

    /* wait for commands */
	while(1) {
		tFD fd;

		/* move mouse-packages */
		rb_move(rbuf,ibuf,rb_length(ibuf));
		if(rb_length(rbuf) > 0)
			setDataReadable(sid,true);

		fd = getClient(&sid,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_READ: {
						/* offset is ignored here */
						u32 count = msg.args.arg2 / sizeof(sMouseData);
						sMouseData *buffer = (sMouseData*)malloc(count * sizeof(sMouseData));
						msg.args.arg1 = 0;
						if(buffer)
							msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sMouseData);
						msg.args.arg2 = rb_length(rbuf) > 0;
						send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
						if(buffer) {
							send(fd,MSG_DRV_READ_RESP,buffer,count * sizeof(sMouseData));
							free(buffer);
						}
					}
					break;
					case MSG_DRV_WRITE:
						msg.args.arg1 = ERR_UNSUPPORTED_OPERATION;
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_IOCTL: {
						msg.data.arg1 = ERR_UNSUPPORTED_OPERATION;
						send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;
					case MSG_DRV_CLOSE:
						break;
				}
			}
			close(fd);
		}
	}

	/* cleanup */
	rb_destroy(ibuf);
	rb_destroy(rbuf);
	releaseIOPort(IOPORT_KB_CTRL);
	releaseIOPort(IOPORT_KB_DATA);
	unsetSigHandler(SIG_INTRPT_MOUSE);
	unregService(sid);
	return EXIT_SUCCESS;
}

static void irqHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	/* check if there is mouse-data */
	u8 status = inByte(IOPORT_KB_CTRL);
	if(!(status & KBC_STATUS_MOUSE_DATA_AVAIL))
		return;

	switch(byteNo) {
		case 0:
			packet.status.all = inByte(IOPORT_KB_DATA);
			byteNo++;
			break;
		case 1:
			packet.xcoord = inByte(IOPORT_KB_DATA);
			byteNo++;
			break;
		case 2:
			packet.ycoord = inByte(IOPORT_KB_DATA);
			byteNo = 0;
			/* write the message in our receive-pipe*/
			mdata.x = packet.xcoord;
			mdata.y = packet.ycoord;
			mdata.buttons = (packet.status.leftBtn << 2) |
				(packet.status.rightBtn << 1) |
				(packet.status.middleBtn << 0);
			rb_write(ibuf,&mdata);
			break;
	}
}

static void kb_init(void) {
	/* activate mouse */
	outByte(IOPORT_KB_CTRL,KBC_CMD_ENABLE_MOUSE);
	kb_checkCmd();

	/* put mouse in streaming mode */
	kb_writeMouse(MOUSE_CMD_STREAMING);

	/* read cmd byte */
	outByte(IOPORT_KB_CTRL,KBC_CMD_READ_STATUS);
	kb_checkCmd();
	u8 cmdByte = kb_read();
	outByte(IOPORT_KB_CTRL,KBC_CMD_SET_STATUS);
	kb_checkCmd();
	cmdByte |= KBC_CMDBYTE_ENABLE_IRQ12 | KBC_CMDBYTE_ENABLE_IRQ1;
	cmdByte &= ~KBC_CMDBYTE_DISABLE_KB;
	outByte(IOPORT_KB_DATA,cmdByte);
	kb_checkCmd();
}

static void kb_checkCmd(void) {
	while(inByte(IOPORT_KB_CTRL) & KBC_STATUS_BUSY);
}

static u16 kb_read(void) {
	u16 c = 0;
	u8 status;
	while(c++ < 0xFFFF && !((status = inByte(IOPORT_KB_CTRL)) & KBC_STATUS_DATA_AVAIL));
	if(!(status & KBC_STATUS_DATA_AVAIL))
		return 0xFF00;
	return inByte(IOPORT_KB_DATA);
}

static u16 kb_readMouse(void) {
	u16 c = 0;
	u8 status;
	while(c++ < 0xFFFF && !((status = inByte(IOPORT_KB_CTRL)) & KBC_STATUS_MOUSE_DATA_AVAIL));
	if(!(status & KBC_STATUS_MOUSE_DATA_AVAIL))
		return 0xFF00;
	return inByte(IOPORT_KB_DATA);
}

static u16 kb_writeMouse(u8 cmd) {
	outByte(IOPORT_KB_CTRL,KBC_CMD_NEXT2MOUSE);
	kb_checkCmd();
	outByte(IOPORT_KB_DATA,cmd);
	kb_checkCmd();
	return kb_read();
}
