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

#include <common.h>
#include <arch/x86/gdt.h>
#include <arch/x86/lapic.h>

class SMP : public SMPBase {
	friend class SMPBase;

	SMP() = delete;

public:
	/**
	 * @param logId the logical id
	 * @return the physical id of the CPU with given logical id
	 */
	static cpuid_t getPhysId(cpuid_t logId);

	/**
	 * Tells the BSP that an AP is running
	 */
	static void apIsRunning();

private:
	static cpuid_t *log2Phys;
};

inline cpuid_t SMP::getPhysId(cpuid_t logId) {
	return log2Phys[logId];
}

inline void SMPBase::sendIPI(cpuid_t id,uint8_t vector) {
	LAPIC::sendIPITo(SMP::getPhysId(id),vector);
}

inline cpuid_t SMPBase::getCurId() {
	return GDT::getCPUId();
}
