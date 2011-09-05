/**
 * $Id: timer.c 227 2011-06-11 18:40:58Z nasmussen $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "common.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "dev/timer.h"
#include "mmix/mem.h"
#include "mmix/error.h"
#include "mmix/io.h"
#include "exception.h"
#include "event.h"
#include "config.h"
#include "sim.h"

#define TIMER_CTRL			0				// timer control register
#define TIMER_DIVISOR		8				// timer divisor register
#define TIMER_EXP			0x01			// timer has expired
#define TIMER_IEN			0x02			// enable timer interrupt

#define TIME_WRAP			1000000000000	// avoid overflow of current time

typedef struct timer {
	struct timer *next;
	octa alarm;
	const sDevice *device;
	fTimerCallback callback;
	int param;
} sTimer;

static void timer_reset(void);
static octa timer_read(octa addr,bool sideEffects);
static void timer_write(octa addr,octa data);
static void timer_shutdown(void);

static sDevice timerDev;
static sTimer *activeTimers = NULL;
static sTimer *freeTimers = NULL;

static octa currentTime;

static octa timerCtrl;
static octa timerDivisor;
static octa timerCounter;

void timer_init(void) {
	// register device
	timerDev.name = "Timer";
	timerDev.startAddr = TIMER_START_ADDR;
	timerDev.endAddr = TIMER_START_ADDR + 2 * sizeof(octa) - 1;
	timerDev.irqMask = (octa)1 << TIMER_IRQ;
	timerDev.reset = timer_reset;
	timerDev.read = timer_read;
	timerDev.write = timer_write;
	timerDev.shutdown = timer_shutdown;
	bus_register(&timerDev);

	for(int i = 0; i < TIMER_COUNT; i++) {
		sTimer *timer = mem_alloc(sizeof(sTimer));
		timer->next = freeTimers;
		freeTimers = timer;
	}
	timer_reset();
}

static void timer_reset(void) {
	currentTime = 0;
	timerCtrl = 0x0000000000000000;
	timerDivisor = 0xFFFFFFFFFFFFFFFF;
	timerCounter = 0xFFFFFFFFFFFFFFFF;
	while(activeTimers != NULL) {
		sTimer *timer = activeTimers;
		activeTimers = timer->next;
		timer->next = freeTimers;
		freeTimers = timer;
	}
}

static void timer_shutdown(void) {
	timer_reset();
	while(freeTimers != NULL) {
		sTimer *timer = freeTimers;
		freeTimers = timer->next;
		mem_free(timer);
	}
}

void timer_tick(void) {
	// increment current time, avoid overflow
	if(++currentTime == TIME_WRAP) {
		currentTime -= TIME_WRAP;
		sTimer *timer = activeTimers;
		while(timer != NULL) {
			timer->alarm -= TIME_WRAP;
			timer = timer->next;
		}
	}
	// check whether any simulation timer expired
	while(activeTimers != NULL && currentTime >= activeTimers->alarm) {
		sTimer *timer = activeTimers;
		activeTimers = timer->next;
		ev_fire2(EV_DEV_CALLBACK,(octa)timer->device,timer->param);
		timer->callback(timer->param);
		timer->next = freeTimers;
		freeTimers = timer;
	}
	// decrement counter and check if an interrupt must be raised
	if(--timerCounter == 0) {
		timerCounter = timerDivisor;
		timerCtrl |= TIMER_EXP;
		// raise timer interrupt
		if(timerCtrl & TIMER_IEN)
			cpu_setInterrupt(TIMER_IRQ);
	}
}

void timer_start(octa msec,const sDevice *device,fTimerCallback callback,int param) {
	if(freeTimers == NULL)
		sim_error("out of timers");

	sTimer *timer = freeTimers;
	freeTimers = timer->next;
	timer->alarm = currentTime + msec;
	timer->device = device;
	timer->callback = callback;
	timer->param = param;
	if(activeTimers == NULL || timer->alarm < activeTimers->alarm) {
		// link into front of active timers queue
		timer->next = activeTimers;
		activeTimers = timer;
	}
	else {
		// link elsewhere into active timers queue
		sTimer *p = activeTimers;
		while(p->next != NULL && p->next->alarm <= timer->alarm) {
			p = p->next;
		}
		timer->next = p->next;
		p->next = timer;
	}
}

static octa timer_read(octa addr,bool sideEffects) {
	UNUSED(sideEffects);
	octa data = 0;
	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case TIMER_CTRL:
			data = timerCtrl;
			break;

		case TIMER_DIVISOR:
			data = timerDivisor;
			break;

		default:
			// illegal register
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
	return data;
}

static void timer_write(octa addr,octa data) {
	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case TIMER_CTRL:
			if(data & TIMER_IEN)
				timerCtrl |= TIMER_IEN;
			else
				timerCtrl &= ~TIMER_IEN;
			if(data & TIMER_EXP)
				timerCtrl |= TIMER_EXP;
			else
				timerCtrl &= ~TIMER_EXP;
			// raise/lower timer interrupt
			if((timerCtrl & TIMER_IEN) != 0 && (timerCtrl & TIMER_EXP) != 0)
				cpu_setInterrupt(TIMER_IRQ);
			else
				cpu_resetInterrupt(TIMER_IRQ);
			break;

		case TIMER_DIVISOR:
			timerDivisor = data;
			timerCounter = data;
			break;

		default:
			// illegal register
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
}
