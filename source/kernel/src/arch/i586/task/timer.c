/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/arch/i586/ports.h>
#include <sys/task/timer.h>
#include <sys/cpu.h>

#define TOLERANCE				1000000
#define MEASURE_COUNT			10
#define REQUIRED_MATCHES		5

#define TIMER_BASE_FREQUENCY	1193182
#define IOPORT_TIMER_CTRL		0x43
#define IOPORT_TIMER_CHAN0DIV	0x40

/* counter to select */
#define TIMER_CTRL_CHAN0		0x00
#define TIMER_CTRL_CHAN1		0x40
#define TIMER_CTRL_CHAN2		0x80
/* read/write mode */
#define TIMER_CTRL_RWLO			0x10	/* low byte only */
#define TIMER_CTRL_RWHI			0x20	/* high byte only */
#define TIMER_CTRL_RWLOHI		0x30	/* low byte first, then high byte */
/* mode */
#define TIMER_CTRL_MODE1		0x02	/* programmable one shot */
#define TIMER_CTRL_MODE2		0x04	/* rate generator */
#define TIMER_CTRL_MODE3		0x06	/* square wave generator */
#define TIMER_CTRL_MODE4		0x08	/* software triggered strobe */
#define TIMER_CTRL_MODE5		0x0A	/* hardware triggered strobe */
/* count mode */
#define TIMER_CTRL_CNTBIN16		0x00	/* binary 16 bit */
#define TIMER_CTRL_CNTBCD		0x01	/* BCD */

static uint64_t timer_determineSpeed(int instrCount);

static uint64_t cpuMhz;

void timer_arch_init(void) {
	/* change timer divisor */
	uint freq = TIMER_BASE_FREQUENCY / TIMER_FREQUENCY_DIV;
	ports_outByte(IOPORT_TIMER_CTRL,TIMER_CTRL_CHAN0 | TIMER_CTRL_RWLOHI |
			TIMER_CTRL_MODE2 | TIMER_CTRL_CNTBIN16);
	ports_outByte(IOPORT_TIMER_CHAN0DIV,freq & 0xFF);
	ports_outByte(IOPORT_TIMER_CHAN0DIV,freq >> 8);
}

uint64_t timer_cyclesToTime(uint64_t cycles) {
	return cycles / cpuMhz;
}

uint64_t timer_detectCPUSpeed(void) {
	int i,j;
	int bestMatches = -1;
	uint64_t bestHz = 0;
	for(i = 1; i <= MEASURE_COUNT; i++) {
		bool found = true;
		uint64_t ref = timer_determineSpeed(0x1000 * i * 10);
		/* no tick at all? */
		if(ref == 0)
			continue;

		for(j = 0; j < REQUIRED_MATCHES; j++) {
			uint64_t hz = timer_determineSpeed(0x1000 * i * 10);
			/* not in tolerance? */
			if(((ref - hz) > 0 && (ref - hz) > TOLERANCE) ||
				((hz - ref) > 0 && (hz - ref) > TOLERANCE)) {
				found = false;
				break;
			}
		}
		if(found) {
			bestHz = ref;
			break;
		}
		/* store our best result */
		if(j > bestMatches) {
			bestMatches = j;
			bestHz = ref;
		}
	}
	/* ok give up and use the best result so far */
	cpuMhz = bestHz / 1000000;
	return bestHz;
}

static uint64_t timer_determineSpeed(int instrCount) {
	uint64_t before,after;
	uint32_t left;
	volatile int i;
	/* set PIT channel 0 to single-shot mode */
	ports_outByte(IOPORT_TIMER_CTRL,TIMER_CTRL_CHAN0 | TIMER_CTRL_RWLOHI |
			TIMER_CTRL_MODE2 | TIMER_CTRL_CNTBIN16);
	/* set reload-value to 65535 */
	ports_outByte(IOPORT_TIMER_CHAN0DIV,0xFF);
	ports_outByte(IOPORT_TIMER_CHAN0DIV,0xFF);

	/* now spend some time and measure the number of cycles */
	before = cpu_rdtsc();
	for(i = instrCount; i > 0; i--)
		;
	after = cpu_rdtsc();

	/* read current count */
	ports_outByte(IOPORT_TIMER_CTRL,TIMER_CTRL_CHAN0);
	left = ports_inByte(IOPORT_TIMER_CHAN0DIV);
	left |= ports_inByte(IOPORT_TIMER_CHAN0DIV) << 8;
	/* if there was no tick at all, we have to increase instrCount */
	if(left == 0xFFFF)
		return 0;
	return (after - before) * (TIMER_BASE_FREQUENCY / (0xFFFF - left));
}
