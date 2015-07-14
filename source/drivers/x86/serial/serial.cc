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

#include <esc/ipc/serialdevice.h>
#include <sys/arch/x86/ports.h>
#include <sys/irq.h>
#include <sys/thread.h>

using namespace esc;

class X86SerialTermDevice;

struct ComPort {
	const char *name;
	int irq;
	int base;
};

static const int IOPORT_COUNT	= 8;
static const ComPort ports[] = {
	{"com1", 4, 0x3F8},
	{"com2", 3, 0x2F8},
	{"com3", 4, 0x3E8},
	{"com4", 3, 0x2E8},
};

static size_t port;
static bool run = true;
static X86SerialTermDevice *vtdev;

class X86SerialTermDevice : public SerialTermDevice {
public:
	explicit X86SerialTermDevice(const char *name,mode_t perm) : SerialTermDevice(name,perm) {
	}

	virtual void writeChar(char c) {
		if(c == '\n')
			writeChar('\r');

		while((inbyte(ports[port].base + 5) & 0x20) == 0)
			;
		outbyte(ports[port].base,c);
	}
};

static void sigterm(int) {
	run = false;
	vtdev->stop();
}

static int irqThread(void *arg) {
	SerialTermDevice *dev = (SerialTermDevice*)arg;

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	int sem = semcrtirq(ports[port].irq,ports[port].name,NULL,NULL);
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",ports[port].irq);
	while(run) {
		semdown(sem);

		if(inbyte(ports[port].base + 5) & 1)
			dev->handleChar(inbyte(ports[port].base));
	}
	return 0;
}

int main(int argc,char **argv) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s <port>\n",argv[0]);
		return EXIT_FAILURE;
	}

	port = ARRAY_SIZE(ports);
	for(size_t i = 0; i < ARRAY_SIZE(ports); ++i) {
		if(strcmp(ports[i].name,argv[1]) == 0) {
			port = i;
			break;
		}
	}
	if(port == ARRAY_SIZE(ports))
		error("Unknown port '%s'",argv[1]);

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/dev/%s",argv[1]);

	print("Creating vterm at %s",path);

	/* create device */
	vtdev = new X86SerialTermDevice(path,0777);

	if(reqports(ports[port].base,IOPORT_COUNT) < 0)
		error("Unable to request IO ports %d..%d",ports[port].base,ports[port].base + IOPORT_COUNT - 1);

	outbyte(ports[port].base + 1, 0x00);	// Disable all interrupts
	outbyte(ports[port].base + 3, 0x80);	// Enable DLAB (set baud rate divisor)
	outbyte(ports[port].base + 0, 0x01);	// Set divisor to 1 (lo byte) 115200 baud
	outbyte(ports[port].base + 1, 0x00);	//                  (hi byte)
	outbyte(ports[port].base + 3, 0x03);	// 8 bits, no parity, one stop bit
	outbyte(ports[port].base + 2, 0xC7);	// Enable FIFO, clear them, with 14-byte threshold
	outbyte(ports[port].base + 4, 0x0B);	// IRQs enabled, RTS/DSR set
	outbyte(ports[port].base + 1, 0x01);	// IRQ if data available

	int tid;
	if((tid = startthread(irqThread,vtdev)) < 0)
		error("Unable to start thread for vterm %s",path);

	vtdev->loop();

	join(tid);
	return EXIT_SUCCESS;
}
