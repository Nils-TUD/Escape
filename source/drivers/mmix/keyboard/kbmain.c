/**
 * $Id: kbmain.c 900 2011-06-02 20:18:17Z nasmussen $
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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/ringbuffer.h>
#include <esc/mem.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errors.h>

#include "set2.h"

#define KEYBOARD_BASE		0x0006000000000000

#define KEYBOARD_CTRL		0
#define KEYBOARD_DATA		1

#define KEYBOARD_RDY		0x01
#define KEYBOARD_IEN		0x02

#define BUF_SIZE			128
#define SC_BUF_SIZE			16
#define TIMEOUT				60
#define SLEEP_TIME			20

static void kbIntrptHandler(int sig);

static sMsg msg;
static sRingBuf *rbuf;
static uint64_t *kbRegs;
static uint8_t scBuf[SC_BUF_SIZE];
static size_t scReadPos = 0;
static size_t scWritePos = 0;

int main(void) {
	int id;
	msgid_t mid;

	kbRegs = (uint64_t*)mapPhysical(KEYBOARD_BASE,2 * sizeof(uint64_t));
	if(kbRegs == NULL)
		error("Unable to map keyboard registers");

	/* create buffers */
	rbuf = rb_create(sizeof(sKbData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* we want to get notified about keyboard interrupts */
	if(setSigHandler(SIG_INTRPT_KB,kbIntrptHandler) < 0)
		error("Unable to announce sig-handler for %d",SIG_INTRPT_KB);

	id = regDriver("keyboard",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'keyboard'");

	/* enable interrupts */
	kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;

    /* wait for commands */
	while(1) {
		int fd;

		/* translate scancodes to keycodes */
		if(scReadPos != scWritePos) {
			while(scReadPos != scWritePos) {
				sKbData data;
				if(kb_set2_getKeycode(&data.isBreak,&data.keycode,scBuf[scReadPos])) {
					if(!data.isBreak && data.keycode == VK_F12) {
						debug();
						kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
					}
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

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	rb_destroy(rbuf);
	close(id);
	return EXIT_SUCCESS;
}

static void kbIntrptHandler(int sig) {
	UNUSED(sig);
	scBuf[scWritePos] = kbRegs[KEYBOARD_DATA];
	scWritePos = (scWritePos + 1) % SC_BUF_SIZE;
}
