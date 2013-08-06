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

uint64_t Timer::cpuMhz;

void TimerBase::archInit() {
	/* change timer divisor */
	uint freq = Timer::BASE_FREQUENCY / Timer::FREQUENCY_DIV;
	Ports::out<uint8_t>(Timer::IOPORT_CTRL,Timer::CTRL_CHAN0 | Timer::CTRL_RWLOHI |
	              Timer::CTRL_MODE2 | Timer::CTRL_CNTBIN16);
	Ports::out<uint8_t>(Timer::IOPORT_CHAN0DIV,freq & 0xFF);
	Ports::out<uint8_t>(Timer::IOPORT_CHAN0DIV,freq >> 8);
}

void Timer::wait(uint us) {
	uint64_t start = CPU::rdtsc();
	uint64_t end = start + timeToCycles(us);
	while(CPU::rdtsc() < end)
		__asm__ volatile ("pause");
}

uint64_t Timer::detectCPUSpeed() {
	int i,j;
	int bestMatches = -1;
	uint64_t bestHz = 0;
	for(i = 1; i <= MEASURE_COUNT; i++) {
		bool found = true;
		uint64_t ref = determineSpeed(0x1000 * i * 10);
		/* no tick at all? */
		if(ref == 0)
			continue;

		for(j = 0; j < REQUIRED_MATCHES; j++) {
			uint64_t hz = determineSpeed(0x1000 * i * 10);
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

uint64_t Timer::determineSpeed(int instrCount) {
	uint64_t before,after;
	uint32_t left;
	volatile int i;
	/* set PIT channel 0 to single-shot mode */
	Ports::out<uint8_t>(IOPORT_CTRL,CTRL_CHAN0 | CTRL_RWLOHI |
			CTRL_MODE2 | CTRL_CNTBIN16);
	/* set reload-value to 65535 */
	Ports::out<uint8_t>(IOPORT_CHAN0DIV,0xFF);
	Ports::out<uint8_t>(IOPORT_CHAN0DIV,0xFF);

	/* now spend some time and measure the number of cycles */
	before = CPU::rdtsc();
	for(i = instrCount; i > 0; i--)
		;
	after = CPU::rdtsc();

	/* read current count */
	Ports::out<uint8_t>(IOPORT_CTRL,CTRL_CHAN0);
	left = Ports::in<uint8_t>(IOPORT_CHAN0DIV);
	left |= Ports::in<uint8_t>(IOPORT_CHAN0DIV) << 8;
	/* if there was no tick at all, we have to increase instrCount */
	if(left == 0xFFFF)
		return 0;
	return (after - before) * (BASE_FREQUENCY / (0xFFFF - left));
}
