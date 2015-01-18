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

#include <esc/ipc/clientdevice.h>
#include <esc/proto/input.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/irq.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "set2.h"

#if defined(__eco32__)
#	define KEYBOARD_BASE	0x30200000
#	define KEYBOARD_IRQ		0x4
#else
#	define KEYBOARD_BASE	0x0006000000000000
#	define KEYBOARD_IRQ		0x3D
#endif

#define KEYBOARD_CTRL		0
#define KEYBOARD_DATA		1

#define KEYBOARD_RDY		0x01
#define KEYBOARD_IEN		0x02

#define BUF_SIZE			128
#define SC_BUF_SIZE			16
#define TIMEOUT				60
#define SLEEP_TIME			20

static int kbIrqThread(A_UNUSED void *arg);

static esc::ClientDevice<> *dev = NULL;
static ulong *kbRegs;

int main(void) {
	uintptr_t phys = KEYBOARD_BASE;
	kbRegs = (ulong*)mmapphys(&phys,2 * sizeof(ulong),0,MAP_PHYS_MAP);
	if(kbRegs == NULL)
		error("Unable to map keyboard registers");

	/* enable interrupts */
	kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;

	dev = new esc::ClientDevice<>("/dev/keyb",0110,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CLOSE);
	if(startthread(kbIrqThread,NULL) < 0)
		error("Unable to start irq-thread");
	dev->loop();

	/* clean up */
	munmap(kbRegs);
	return EXIT_SUCCESS;
}

static int kbIrqThread(A_UNUSED void *arg) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	int sem = semcrtirq(KEYBOARD_IRQ,"Keyboard",NULL,NULL);
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",KEYBOARD_IRQ);
	while(1) {
		semdown(sem);

		ulong sc = kbRegs[KEYBOARD_DATA];

		esc::Keyb::Event ev;
		if(kb_set2_getKeycode(&ev.flags,&ev.keycode,sc)) {
			esc::IPCBuf ib(buffer,sizeof(buffer));
			ib << ev;
			try {
				dev->broadcast(esc::Keyb::Event::MID,ib);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
		}

		/* re-enable interrupts; the kernel has disabled it in order to get into userland */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
	return 0;
}
