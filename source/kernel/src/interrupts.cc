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
#include <sys/interrupts.h>
#include <sys/ostream.h>

size_t InterruptsBase::getCount() {
	ulong total = 0;
	for(size_t i = 0; i < IRQ_COUNT; ++i)
		total += intrptList[i].count;
	return total;
}

void InterruptsBase::print(OStream &os) {
	for(size_t i = 0; i < IRQ_COUNT; ++i) {
		const Interrupt *irq = intrptList + i;
		if(strcmp(irq->name,"??") != 0)
			os.writef("%-20s: %lu\n",irq->name,irq->count);
	}
}
