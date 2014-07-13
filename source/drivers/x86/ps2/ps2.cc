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
#include "keyboard.h"
#include "mouse.h"

// port 1 is enabled by default
uint PS2::_ports = PORT1 | PORT2;

bool PS2::waitInbuf() {
	int i;
	for(i = 0; i < TIMEOUT && (inbyte(PORT_CTRL) & ST_INBUF_FULL) != 0; ++i)
		sleep(1);
	if(i == TIMEOUT)
		print("Timeout while waiting for bit ST_INBUF_FULL to clear");
	return i != TIMEOUT;
}

bool PS2::waitOutbuf() {
	int i;
	for(i = 0; i < TIMEOUT && (inbyte(PORT_CTRL) & ST_OUTBUF_FULL) == 0; ++i)
		sleep(1);
	if(i == TIMEOUT)
		print("Timeout while waiting for bit ST_OUTBUF_FULL to clear");
	return i != TIMEOUT;
}

void PS2::ctrlCmd(uint8_t cmd) {
	// print("Sending command %#02x",cmd);
	outbyte(PORT_CTRL,cmd);
}

void PS2::ctrlCmd(uint8_t cmd,uint8_t data) {
	ctrlCmd(cmd);
	waitInbuf();
	// print("Sending data %#02x",data);
	outbyte(PORT_DATA,data);
}

uint8_t PS2::ctrlCmdResp(uint8_t cmd) {
	ctrlCmd(cmd);
	return ctrlRead();
}

uint8_t PS2::ctrlRead() {
	bool res = waitOutbuf();
	uint8_t data = inbyte(PORT_DATA);
	// print("Got response %#02x",data);
	return res ? data : 0xFF;
}

uint8_t PS2::devCmd(uint8_t cmd,uint8_t port) {
	int i;
	// print("Device command %#02x for port %u",cmd,port);
	for(i = 0; i < 2; ++i) {
		if(port == PORT2)
			outbyte(PORT_CTRL,CTRL_CMD_NEXT_PORT2_IN);
		waitInbuf();
		outbyte(PORT_DATA,cmd);
		waitOutbuf();
		uint8_t res = inbyte(PORT_DATA);
		if(res != 0xFE)
			return res;
	}
	if(i == 2)
		print("Timeout while executing device command %#02x",cmd);
	return 0xFE;
}

void PS2::init() {
	if(reqport(PS2::PORT_DATA) < 0)
		error("Unable to request io-port",PS2::PORT_DATA);
	if(reqport(PS2::PORT_CTRL) < 0)
		error("Unable to request io-port",PS2::PORT_CTRL);

	uint8_t data;

	// disable devices first
	ctrlCmd(CTRL_CMD_DIS_PORT1);
	ctrlCmd(CTRL_CMD_DIS_PORT2);

	// empty output buffer
	while(inbyte(PORT_CTRL) & ST_OUTBUF_FULL)
		inbyte(PORT_DATA);

	// set configuration byte to a proper value
	data = ctrlCmdResp(CTRL_CMD_GETCFG);
	if(data == 0xFF)
		data = 0;
	data &= ~(CFG_IRQ_PORT1 | CFG_IRQ_PORT2 | CFG_TRANSLATION);
	ctrlCmd(CTRL_CMD_SETCFG,data);

	// do the self-test
	if(ctrlCmdResp(CTRL_CMD_TEST) != 0x55)
		printe("PS/2 controller test failed");

	// this is not reliable on all devices. better assume that we have 2 ports
#if 0
	// if there is a second port
	if(data & CFG_CLOCK_PORT2) {
		ctrlCmd(CTRL_CMD_EN_PORT2);
		data = ctrlCmdResp(CTRL_CMD_GETCFG);
		if(data == 0xFF)
			data = 0;
		// if we have a second port, disable it again
		if(~data & CFG_CLOCK_PORT2) {
			ctrlCmd(CTRL_CMD_DIS_PORT2);
			_ports |= PORT2;
		}
	}

	// test ports
	if(ctrlCmdResp(CTRL_CMD_TEST_PORT1) != 0x00) {
		printe("PS/2 port 1 not present");
		_ports &= ~PORT1;
	}
	if((_ports & PORT2) && ctrlCmdResp(CTRL_CMD_TEST_PORT2) != 0x00) {
		printe("PS/2 port 2 not present");
		_ports &= ~PORT2;
	}
#endif

	// enable devices
	if(_ports & PORT1) {
		print("Enabling Port 1");
		ctrlCmd(CTRL_CMD_EN_PORT1);
		Keyboard::init();
	}

	if(_ports & PORT2) {
		print("Enabling Port 2");
		ctrlCmd(CTRL_CMD_EN_PORT2);
		Mouse::init();
	}

	data = ctrlCmdResp(CTRL_CMD_GETCFG);
	if(data == 0xFF)
		data = 0;
	if(_ports & PORT1)
		data |= CFG_IRQ_PORT1;
	if(_ports & PORT2)
		data |= CFG_IRQ_PORT2;
	ctrlCmd(CTRL_CMD_SETCFG,data | CFG_TRANSLATION);
}

int main() {
	PS2::init();

	if(startthread(Mouse::run,NULL) < 0)
		error("Unable to start mouse thread");
	Keyboard::run(NULL);
	return 0;
}
