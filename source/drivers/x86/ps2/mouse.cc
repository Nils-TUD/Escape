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

#include <sys/common.h>
#include <sys/arch/x86/ports.h>
#include <sys/thread.h>
#include <sys/irq.h>
#include <esc/proto/input.h>
#include <esc/ipc/clientdevice.h>
#include <stdlib.h>
#include <stdio.h>

#include "ps2.h"
#include "mouse.h"

typedef union {
	uint8_t	yOverflow : 1,
			xOverflow : 1,
			ySign : 1,
			xSign : 1,
			: 1,
			middleBtn : 1,
			rightBtn : 1,
			leftBtn : 1;
	uint8_t all;
} uStatus;

bool Mouse::_wheel = false;

void Mouse::init() {
	print("Putting mouse in streaming mode");
	PS2::devCmd(CMD_STREAMING,PS2::PORT2);

	print("Checking whether we have a wheel");
	// enable mouse-wheel by setting sample-rate to 200, 100 and 80 and reading the device-id
	PS2::devCmd(CMD_SETSAMPLE,PS2::PORT2);
	PS2::devCmd(200,PS2::PORT2);
	PS2::devCmd(CMD_SETSAMPLE,PS2::PORT2);
	PS2::devCmd(100,PS2::PORT2);
	PS2::devCmd(CMD_SETSAMPLE,PS2::PORT2);
	PS2::devCmd(80,PS2::PORT2);
	PS2::devCmd(CMD_GETDEVID,PS2::PORT2);
	uint8_t id = PS2::ctrlRead();
	if(id == 3 || id == 4) {
		print("Detected mouse-wheel");
		_wheel = true;
	}
}

int Mouse::run(void*) {
	esc::ClientDevice<> dev("/dev/mouse",0110,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CLOSE);

	if(startthread(irqThread,&dev) < 0)
		error("Unable to start irq-thread");

	dev.loop();
	return 0;
}

int Mouse::irqThread(void *arg) {
	static int byteNo = 0;
	esc::ClientDevice<> *dev = (esc::ClientDevice<>*)arg;
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::Mouse::Event ev;

	int sem = semcrtirq(PS2::IRQ_MOUSE,"PS/2 Mouse",NULL,NULL);
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",PS2::IRQ_MOUSE);
	while(1) {
		semdown(sem);

		// check if there is mouse-data
		uint8_t status = inbyte(PS2::PORT_CTRL);
		if(!(status & PS2::ST_OUTBUF_FULL_PORT2))
			continue;

		int8_t data = inbyte(PS2::PORT_DATA);
		switch(byteNo) {
			case 0: {
				uStatus st;
				st.all = data;
				ev.buttons = (st.leftBtn << 2) | (st.rightBtn << 1) | (st.middleBtn << 0);
				byteNo++;
			}
			break;
			case 1:
				ev.x = data;
				byteNo++;
				break;
			case 2:
				ev.y = data;
				if(_wheel)
					byteNo++;
				else {
					byteNo = 0;
					ev.z = 0;
				}
				break;
			case 3:
				ev.z = data;
				byteNo = 0;
				break;
		}

		if(byteNo == 0) {
			esc::IPCBuf ib(buffer,sizeof(buffer));
			ib << ev;
			try {
				dev->broadcast(esc::Mouse::Event::MID,ib);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
		}
	}
}
