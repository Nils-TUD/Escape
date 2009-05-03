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
#include <esc/messages.h>
#include <esc/service.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/signals.h>
#include <esc/lock.h>
#include <stdlib.h>
#include <string.h>

#include "set1.h"

/* io-ports */
#define IOPORT_PIC					0x20
#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64

#define KBC_CMD_DISABLE_MOUSE		0xA7
#define KBC_CMD_ENABLE_MOUSE		0xA8

#define KBC_STATUS_DATA_AVAIL		(1 << 0)
#define KBC_STATUS_BUSY				(1 << 1)
#define KBC_STATUS_MOUSE_DATA_AVAIL	(1 << 5)

/* ICW = initialisation command word */
#define PIC_ICW1					0x20

#define KB_MOUSE_LOCK				0x1

/**
 * The keyboard-interrupt handler
 */
static void kbIntrptHandler(tSig sig,u32 data);
static u16 kb_read(void);
static void kb_checkCmd(void);

/* file-descriptor for ourself */
static tFD selfFd;

int main(void) {
	tServ id;

	id = regService("keyboard",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printe("Unable to register service 'keyboard'");
		return EXIT_FAILURE;
	}

	/* request io-ports */
	if(requestIOPort(IOPORT_PIC) < 0) {
		printe("Unable to request io-port %d",IOPORT_PIC);
		return EXIT_FAILURE;
	}
	if(requestIOPort(IOPORT_KB_DATA) < 0) {
		printe("Unable to request io-port",IOPORT_KB_DATA);
		return EXIT_FAILURE;
	}
	if(requestIOPort(IOPORT_KB_CTRL) < 0) {
		printe("Unable to request io-port",IOPORT_KB_CTRL);
		return EXIT_FAILURE;
	}

	/* open ourself to write keycodes into the receive-pipe (which can be read by other processes) */
	selfFd = open("services:/keyboard",IO_WRITE);
	if(selfFd < 0) {
		printe("Unable to open services:/keyboard");
		return EXIT_FAILURE;
	}

	/* we want to get notified about keyboard interrupts */
	if(setSigHandler(SIG_INTRPT_KB,kbIntrptHandler) < 0) {
		printe("Unable to announce sig-handler for %d",SIG_INTRPT_KB);
		return EXIT_FAILURE;
	}

    /* reset keyboard */
    outByte(IOPORT_KB_DATA,0xff);
    while(inByte(IOPORT_KB_DATA) != 0xaa)
    	yield();

	/* we don't want to be waked up. we'll get signals anyway */
	while(1)
		wait(EV_NOEVENT);

	/* clean up */
	unsetSigHandler(SIG_INTRPT_KB);
	close(selfFd);
	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_DATA);
	releaseIOPort(IOPORT_KB_CTRL);
	unregService(id);

	return EXIT_SUCCESS;
}

static void kbIntrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	static sMsgKbResponse resp;
	if(!(inByte(IOPORT_KB_CTRL) & KBC_STATUS_DATA_AVAIL))
		return;
	u8 scanCode = inByte(IOPORT_KB_DATA);
	if(kb_set1_getKeycode(&resp,scanCode)) {
		/* write in receive-pipe */
		write(selfFd,&resp,sizeof(sMsgKbResponse));
	}
	/* ack scancode
	outByte(IOPORT_PIC,PIC_ICW1);*/
}

static u16 kb_read(void) {
	u16 c = 0;
	u8 status;
	while(c++ < 0xFFFF && !((status = inByte(IOPORT_KB_CTRL)) & KBC_STATUS_DATA_AVAIL));
	if(!(status & KBC_STATUS_DATA_AVAIL))
		return 0xFF00;
	return inByte(IOPORT_KB_DATA);
}

static void kb_checkCmd(void) {
	while(inByte(IOPORT_KB_CTRL) & KBC_STATUS_BUSY);
}
