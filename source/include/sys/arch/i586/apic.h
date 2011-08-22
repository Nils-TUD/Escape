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

#ifndef APIC_H_
#define APIC_H_

#include <sys/common.h>

void apic_init(void);

cpuid_t apic_getId(void);

bool apic_isAvailable(void);
void apic_enable(void);

void apic_sendIPITo(cpuid_t id,uint8_t vector);
void apic_sendInitIPI(void);
void apic_sendStartupIPI(uintptr_t startAddr);
void apic_eoi(void);

#endif /* APIC_H_ */
