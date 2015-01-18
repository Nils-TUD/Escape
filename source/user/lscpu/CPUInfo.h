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

#include <info/cpu.h>
#include <sys/common.h>
#include <stdio.h>

class CPUInfo {
public:
	static CPUInfo *create();

	explicit CPUInfo() {
	}
	virtual ~CPUInfo() {
	}

	virtual void print(FILE *f,info::cpu &cpu) = 0;
};

#if defined(__x86__)
#	include "arch/x86/X86CPUInfo.h"
#elif defined(__eco32__)
#	include "arch/eco32/ECO32CPUInfo.h"
#elif defined(__mmix__)
#	include "arch/mmix/MMIXCPUInfo.h"
#endif

inline CPUInfo *CPUInfo::create() {
#if defined(__x86__)
	return new X86CPUInfo();
#elif defined(__eco32__)
	return new ECO32CPUInfo();
#elif defined(__mmix__)
	return new MMIXCPUInfo();
#endif
}
