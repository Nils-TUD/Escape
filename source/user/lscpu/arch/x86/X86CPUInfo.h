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

class X86CPUInfo : public CPUInfo {
	static const size_t VENDOR_STRLEN				= 12;

	struct Info {
		uint8_t vendor;
		uint16_t model;
		uint16_t family;
		uint16_t type;
		uint16_t brand;
		uint16_t stepping;
		uint32_t signature;
		uint32_t features;
		uint32_t name[12];
	};

public:
	explicit X86CPUInfo() : CPUInfo() {
	}

	virtual void print(FILE *f,info::cpu &cpu);

private:
	Info getInfo() const;
	void cpuid(unsigned code,uint32_t *eax,uint32_t *ebx,uint32_t *ecx,uint32_t *edx) const;
	void cpuid(unsigned code,char *str) const;
	void printFlags(FILE *f) const;
};
