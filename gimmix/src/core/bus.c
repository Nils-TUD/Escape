/**
 * $Id: bus.c 218 2011-06-04 15:46:40Z nasmussen $
 */

#include <assert.h>

#include "common.h"
#include "core/bus.h"
#include "mmix/mem.h"
#include "mmix/error.h"
#include "dev/timer.h"
#include "dev/term.h"
#include "dev/ram.h"
#include "dev/disk.h"
#include "dev/output.h"
#include "dev/rom.h"
#include "dev/dspkbd.h"
#include "exception.h"
#include "sim.h"
#include "event.h"

typedef struct sDeviceNode {
	const sDevice *dev;
	struct sDeviceNode *next;
} sDeviceNode;

static const sDevice *bus_overlapMem(octa start,octa end);

static sDeviceNode *devices = NULL;
static sDeviceNode *lastDevice = NULL;

void bus_init(void) {
	ram_init();
	timer_init();
	term_init();
	disk_init();
	output_init();
	rom_init();
	dspkbd_init();
}

void bus_register(const sDevice *dev) {
	// check for overlapping irqs and memory
	if(bus_getIntrptBits() & dev->irqMask) {
		sim_error("The irq-mask of device '%s' (%016OX) overlaps with existing devices (%016OX)",
				dev->name,dev->irqMask,bus_getIntrptBits());
	}
	const sDevice *overlap = bus_overlapMem(dev->startAddr,dev->endAddr);
	if(overlap) {
		sim_error("The memory-address-range of device '%s' (%016OX..%016OX) overlaps with device"
				" '%s' (%016OX..%016OX)",dev->name,dev->startAddr,dev->endAddr,overlap->name,
				overlap->startAddr,overlap->endAddr);
	}

	sDeviceNode *node = (sDeviceNode*)mem_alloc(sizeof(sDeviceNode));
	node->dev = dev;
	node->next = NULL;
	if(!devices)
		devices = node;
	if(lastDevice)
		lastDevice->next = node;
	lastDevice = node;
}

void bus_reset(void) {
	sDeviceNode *node = devices;
	while(node != NULL) {
		node->dev->reset();
		node = node->next;
	}
}

void bus_shutdown(void) {
	sDeviceNode *node = devices;
	while(node != NULL) {
		sDeviceNode *next = node->next;
		node->dev->shutdown();
		mem_free(node);
		node = next;
	}
}

octa bus_getIntrptBits(void) {
	octa bits = 0;
	sDeviceNode *node = devices;
	while(node != NULL) {
		bits |= node->dev->irqMask;
		node = node->next;
	}
	return bits;
}

const sDevice *bus_getDevByIndex(int index) {
	sDeviceNode *node = devices;
	while(node != NULL) {
		if(index-- == 0)
			return node->dev;
		node = node->next;
	}
	return NULL;
}

const sDevice *bus_getDevByAddr(octa addr) {
	sDeviceNode *node = devices;
	while(node != NULL) {
		if(addr >= node->dev->startAddr && addr <= node->dev->endAddr)
			return node->dev;
		node = node->next;
	}
	return NULL;
}

octa bus_read(octa addr,bool sideEffects) {
	addr &= ~(octa)(sizeof(octa) - 1);
	const sDevice *dev = bus_getDevByAddr(addr);
	if(dev == NULL)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	octa data = dev->read(addr,sideEffects);
	if(sideEffects)
		ev_fire2(EV_DEV_READ,addr,data);
	return data;
}

void bus_write(octa addr,octa data,bool sideEffects) {
	addr &= ~(octa)(sizeof(octa) - 1);
	const sDevice *dev = bus_getDevByAddr(addr);
	if(dev == NULL)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	dev->write(addr,data);
	if(sideEffects)
		ev_fire2(EV_DEV_WRITE,addr,data);
}

static const sDevice *bus_overlapMem(octa start,octa end) {
	sDeviceNode *node = devices;
	while(node != NULL) {
		// start-address in range, end-address in range or range in range
		if((node->dev->startAddr >= start && node->dev->startAddr <= end) ||
			(node->dev->endAddr >= start && node->dev->endAddr <= end) ||
			(node->dev->startAddr < start && node->dev->endAddr > end))
			return node->dev;
		node = node->next;
	}
	return NULL;
}
