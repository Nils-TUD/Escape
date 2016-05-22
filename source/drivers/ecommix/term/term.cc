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
#include <sys/irq.h>
#include <sys/thread.h>

using namespace esc;

class ECOMMIXTermDevice;

#define MAX_TERMINALS		2

#if defined(__eco32__)
#	define TERM_BASE		0x30300000			/* base address for all terminals */
#	define TERM_XMTR_IRQ	0					/* transmitter interrupt for terminal 0 */
#	define TERM_RCVR_IRQ	1					/* receiver interrupt for terminal 0 */
#	define TERM_SIZE		16					/* 16 bytes for each terminal */
#else
#	define TERM_BASE		0x0002000000000000	/* base address for all terminals */
#	define TERM_XMTR_IRQ	0x35				/* transmitter interrupt for terminal 0 */
#	define TERM_RCVR_IRQ	0x36				/* receiver interrupt for terminal 0 */
#	define TERM_SIZE		32					/* 32 bytes for each terminal */
#endif

#define TERM_RCVR_CTRL		0		/* receiver control register */
#define TERM_RCVR_DATA		1		/* receiver data register */
#define TERM_XMTR_CTRL		2		/* transmitter control register */
#define TERM_XMTR_DATA		3		/* transmitter data register */

#define TERM_RCVR_RDY		0x01	/* receiver has a character */
#define TERM_RCVR_IEN		0x02	/* enable receiver interrupt */

#define TERM_XMTR_RDY		0x01	/* transmitter accepts a character */
#define TERM_XMTR_IEN		0x02	/* enable transmitter interrupt */

static volatile ulong *regs;
static size_t dev;
static bool run = true;
static ECOMMIXTermDevice *vtdev;

class ECOMMIXTermDevice : public SerialTermDevice {
public:
	explicit ECOMMIXTermDevice(const char *name,mode_t perm) : SerialTermDevice(name,perm) {
		_xtmrIrq = semcrtirq(TERM_XMTR_IRQ + 2 * dev,name,NULL,NULL);
		if(_xtmrIrq < 0)
			error("Unable to get irq-semaphore for IRQ %d",TERM_XMTR_IRQ + 2 * dev);
	}

	virtual void writeChar(char c) {
		if(c == '\n')
			writeChar('\r');

		semdown(_xtmrIrq);
		regs[TERM_XMTR_CTRL] = TERM_XMTR_IEN;
		regs[TERM_XMTR_DATA] = c;
	}

private:
	int _xtmrIrq;
};

static void sigterm(int) {
	run = false;
	vtdev->stop();
}

static int irqThread(void *arg) {
	ECOMMIXTermDevice *vt = (ECOMMIXTermDevice*)arg;

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	char name[16];
	snprintf(name,sizeof(name),"trcvr%zu",dev);
	int sem = semcrtirq(TERM_RCVR_IRQ + 2 * dev,name,NULL,NULL);
	if(sem < 0)
		error("Unable to get irq-semaphore for IRQ %d",TERM_RCVR_IRQ + 2 * dev);
	while(run) {
		semdown(sem);

		if(regs[TERM_RCVR_CTRL] & TERM_RCVR_RDY)
			vt->handleChar(regs[TERM_RCVR_DATA]);
		/* the kernel has disabled interrupts */
		regs[TERM_RCVR_CTRL] = TERM_RCVR_IEN;
	}
	return 0;
}

int main(int argc,char **argv) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s <dev>\n",argv[0]);
		return EXIT_FAILURE;
	}

	dev = strtoul(argv[1],NULL,0);
	if(dev >= MAX_TERMINALS)
		error("Invalid device number '%s'",dev);

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/dev/term%s",argv[1]);

	uintptr_t phys = TERM_BASE + TERM_SIZE * dev;
	regs = (volatile ulong*)mmapphys(&phys,TERM_SIZE,0,MAP_PHYS_MAP);
	if(regs == NULL)
		error("Unable to request device memory");
	/* walk to our terminal */
	regs += dev * TERM_SIZE / sizeof(ulong);

	/* create device */
	vtdev = new ECOMMIXTermDevice(path,0700);

	/* init terminal */
	regs[TERM_RCVR_CTRL] = TERM_RCVR_IEN;
	regs[TERM_XMTR_CTRL] = TERM_XMTR_RDY | TERM_XMTR_IEN;

	print("Creating vterm at %s",path);

	int tid;
	if((tid = startthread(irqThread,vtdev)) < 0)
		error("Unable to start thread for vterm %s",path);

	vtdev->loop();

	join(tid);
	return EXIT_SUCCESS;
}
