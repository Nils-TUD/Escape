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
#include <esc/arch/x86/ports.h>
#include <esc/keycodes.h>
#include <esc/thread.h>
#include <esc/irq.h>
#include <ipc/proto/input.h>
#include <ipc/clientdevice.h>
#include <stdlib.h>
#include <stdio.h>

#include "ps2.h"
#include "set1.h"
#include "keyboard.h"

uint8_t Keyboard::_leds = 0;

void Keyboard::init() {
	// clear LEDs
	PS2::devCmd(CMD_LEDS,PS2::PORT1);
	PS2::devCmd(_leds,PS2::PORT1);

	// set repeat rate and delay
	Typematic typematic;
	typematic.repeatRate = 7;
	typematic.delay = 3;		// 1000ms
	typematic.zero = 0;
	PS2::devCmd(CMD_TYPEMATIC,PS2::PORT1);
	PS2::devCmd(typematic.value,PS2::PORT1);
}

void Keyboard::updateLEDs(const ipc::Keyb::Event &ev) {
	if(ev.flags & ipc::Keyb::Event::FL_BREAK) {
		uint8_t oldLeds = _leds;
		if(ev.keycode == VK_CAPS)
			_leds ^= LED_CAPS_LOCK;
		else if(ev.keycode == VK_NUM)
			_leds ^= LED_NUM_LOCK;
		else if(ev.keycode == VK_SCROLL)
			_leds ^= LED_SCROLL_LOCK;

		if(_leds != oldLeds) {
			PS2::devCmd(CMD_LEDS,PS2::PORT1);
			PS2::devCmd(_leds,PS2::PORT1);
		}
	}
}

int Keyboard::run(void*) {
	ipc::ClientDevice<> dev("/dev/keyb",0110,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CLOSE);

	if(startthread(irqThread,&dev) < 0)
		error("Unable to start IRQ-thread");

	dev.loop();
	return 0;
}

int Keyboard::irqThread(void *arg) {
	ipc::ClientDevice<> *dev = (ipc::ClientDevice<>*)arg;
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	int sem = semcrtirq(PS2::IRQ_KB,"PS/2 Keyboard",NULL,NULL);
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",PS2::IRQ_KB);
	while(1) {
		semdown(sem);

		uint8_t status = inbyte(PS2::PORT_CTRL);
		if(!(status & PS2::ST_OUTBUF_FULL))
			continue;

		uint8_t sc = inbyte(PS2::PORT_DATA);
		ipc::Keyb::Event ev;
		if(ScancodeSet1::getKeycode(&ev.flags,&ev.keycode,sc,_leds)) {
			ipc::IPCBuf ib(buffer,sizeof(buffer));
			ib << ev;
			try {
				dev->broadcast(ipc::Keyb::Event::MID,ib);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}

			updateLEDs(ev);
		}
	}
	return 0;
}
