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

#pragma once

#include <sys/common.h>
#include <sys/mem/physmem.h>
#include <sys/interrupts.h>
#include <assert.h>

class Timer;

class LAPIC {
	LAPIC() = delete;

	friend class Timer;

	enum Register {
		REG_APICID				= 0x20,
		REG_SPURINT				= 0xF0,
		REG_ICR_LOW				= 0x300,
		REG_ICR_HIGH			= 0x310,
		REG_LVT_TIMER			= 0x320,
		REG_LVT_THERMAL			= 0x330,
		REG_LVT_PERFCNT			= 0x340,
		REG_LVT_LINT0			= 0x350,
		REG_LVT_LINT1			= 0x360,
		REG_LVT_ERROR			= 0x370,
		REG_TIMER_ICR			= 0x380,
		REG_TIMER_CCR			= 0x390,
		REG_TIMER_DCR			= 0x3E0,
		REG_TASK_PRIO			= 0x80,
		REG_EOI					= 0xB0
	};

	enum {
		SPURINT_APIC_EN			= 1 << 8
	};

	enum DeliveryMode {
		ICR_DELMODE_FIXED		= 0x0 << 8,
		ICR_DELMODE_LOWPRIO		= 0x1 << 8,
		ICR_DELMODE_SMI			= 0x2 << 8,
		ICR_DELMODE_NMI			= 0x4 << 8,
		ICR_DELMODE_INIT		= 0x5 << 8,
		ICR_DELMODE_SIPI		= 0x6 << 8,
		ICR_DELMODE_EXTINT		= 0x7 << 8
	};

	enum {
		ICR_DEST_PHYS			= 0x0 << 11,
		ICR_DEST_LOG			= 0x1 << 11
	};

	enum {
		ICR_DELSTAT_IDLE		= 0x0 << 12,
		ICR_DELSTAT_PENDING		= 0x1 << 12
	};

	enum {
		ICR_LEVEL_DEASSERT		= 0x0 << 14,
		ICR_LEVEL_ASSERT		= 0x1 << 14
	};

	enum {
		ICR_TRIGMODE_EDGE		= 0x0 << 15,
		ICR_TRIGMODE_LEVEL		= 0x1 << 15
	};

	enum Mask {
		UNMASKED				= 0x0 << 16,
		MASKED					= 0x1 << 16
	};

	enum Mode {
		MODE_ONESHOT			= 0x0 << 17,
		MODE_PERIODIC			= 0x1 << 17,
		MODE_TSCDEADLINE		= 0x2 << 17,
	};

	enum {
		ICR_DESTSHORT_NO		= 0x0 << 18,
		ICR_DESTSHORT_SELF		= 0x1 << 18,
		ICR_DESTSHORT_ALL		= 0x2 << 18,
		ICR_DESTSHORT_NOTSELF	= 0x3 << 18
	};

	static const uint32_t MSR_APIC_BASE			= 0x1B;
	static const uint32_t APIC_BASE_EN			= 1 << 11;

public:
	static const int TIMER_DIVIDER				= 16;

	static void init();

	static cpuid_t getId() {
		if(enabled)
			return read(REG_APICID) >> 24;
		return 0;
	}

	static bool isAvailable() {
		return enabled;
	}
	static void enable() {
		uint32_t svr = read(REG_SPURINT);
		if(!(svr & 0x100))
			write(REG_SPURINT,SPURINT_APIC_EN);
		write(REG_TASK_PRIO,0x10);
		write(REG_TIMER_DCR,0x3);	// set divider to 16
	}
	static void enableTimer();

	static void sendIPITo(cpuid_t id,uint8_t vector) {
		writeIPI(id << 24,ICR_DESTSHORT_NO | ICR_LEVEL_ASSERT |
				ICR_DEST_PHYS | ICR_DELMODE_FIXED | vector);
	}
	static void sendInitIPI() {
		writeIPI(0,ICR_DESTSHORT_NOTSELF | ICR_LEVEL_ASSERT |
				ICR_DEST_PHYS | ICR_DELMODE_INIT);
	}
	static void sendStartupIPI(uintptr_t startAddr) {
		writeIPI(0,ICR_DESTSHORT_NOTSELF | ICR_LEVEL_ASSERT |
				ICR_DEST_PHYS | ICR_DELMODE_SIPI | ((startAddr / PAGE_SIZE) & 0xFF));
	}
	static void eoi() {
		write(REG_EOI,0);
	}

	static uint32_t getCounter() {
		return read(REG_TIMER_CCR);
	}

private:
	static void writeIPI(uint32_t high,uint32_t low);

    static void setLVT(Register reg,uint vector,DeliveryMode dlv,Mask mask = UNMASKED,Mode mode = MODE_ONESHOT) {
        write(reg,dlv | vector | mode | mask);
    }

	static uint32_t read(uint32_t reg) {
		/* volatile is necessary to enforce dword-accesses */
		return *(volatile uint32_t*)(apicAddr + reg);
	}

	static void write(uint32_t reg,uint32_t value) {
		*(volatile uint32_t*)(apicAddr + reg) = value;
	}

	static bool enabled;
	static uintptr_t apicAddr;
};

EXTERN_C void lapic_eoi();
