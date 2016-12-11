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

#include <esc/util.h>
#include <arch/x86/mtrr.h>
#include <mem/physmemareas.h>
#include <dbg/console.h>
#include <log.h>

SpinLock MTRR::_lock;
uint MTRR::_addrbits = 0;
uint64_t MTRR::_cap = 0;

void MTRR::init() {
	// TODO qemu-kvm does not set this bit, but supports MTRRs :(
	// if(!CPU::hasFeature(CPU::BASIC,CPU::FEAT_MTRR))
	// 	return;

	_cap = CPU::getMSR(CPU::MSR_MTRRCAP);

	// determine the physical address bits
	uint32_t eax,dummy;
	CPU::cpuid(CPU::CPUID_MAXPHYSADDR,&eax,&dummy,&dummy,&dummy);
	if(eax)
		_addrbits = eax & 0xFF;
	else
		_addrbits = 36;

	// ensure that MTRRs are enabled
	uint64_t def = CPU::getMSR(CPU::MSR_IA32_MTRR_DEF_TYPE);
	def |= DEF_E;
	CPU::setMSR(CPU::MSR_IA32_MTRR_DEF_TYPE,def);
}

int MTRR::setRange(uint64_t base,uint64_t length,Type type) {
	if(!_cap)
		return -ENOTSUP;

	// is has to be page and size-aligned
	if((length % PAGE_SIZE) != 0 || (base & (length - 1)) != 0)
		return -EINVAL;
	// ensure that we don't exceed the physical address bits
	uint64_t maxphys = ((uint64_t)1 << _addrbits) - 1;
	if(base > maxphys || base + length > maxphys)
		return -EINVAL;

	LockGuard<SpinLock> guard(&_lock);
	// first, check for overlapping areas
	uint count = _cap & CAP_VCNT;
	for(uint i = 0; i < count; ++i) {
		uint64_t tbase = CPU::getMSR(CPU::MSR_IA32_MTRR_PHYSBASE0 + i * 2);
		uint64_t tmask = CPU::getMSR(CPU::MSR_IA32_MTRR_PHYSMASK0 + i * 2);
		uint64_t tstart = tbase & ~(uint64_t)BASE_TYPE;
		uint64_t tlen = ~((tmask & ~(uint64_t)MASK_VALID) - 1) & maxphys;
		// TODO the overlap support is still really limited
		if((tmask & MASK_VALID) && esc::Util::overlap(base,base + length,tstart,tstart + tlen)) {
			// is it the same?
			if(base == tbase && length == tlen && (tbase & BASE_TYPE) == type)
				return 0;

			// it has to be a subset at the beginning for now
			if(tbase != base || tlen < length) {
				Log::get().writef("Overlap not supported: %Lx..%Lx vs. %Lx..%Lx\n",
					base,base + length,tbase,tbase + tlen);
				return -ENOTSUP;
			}

			// ensure that it is still size-aligned
			tbase += length;
			tlen -= length;
			if((tbase & (tlen - 1)) != 0) {
				Log::get().writef("Overlap not supported: %Lx..%Lx vs. %Lx..%Lx\n",
					base,base + length,tbase,tbase + tlen);
				return -ENOTSUP;
			}

			// update the pair
			CPU::setMSR(CPU::MSR_IA32_MTRR_PHYSBASE0 + i * 2,tbase);
			CPU::setMSR(CPU::MSR_IA32_MTRR_PHYSMASK0 + i * 2,(~(tlen - 1) & maxphys) | MASK_VALID);
		}
	}

	// now search for a free pair
	for(uint i = 0; i < count; ++i) {
		uint64_t tmask = CPU::getMSR(CPU::MSR_IA32_MTRR_PHYSMASK0 + i * 2);
		if(tmask & MASK_VALID)
			continue;

		// no, so use that pair
		CPU::setMSR(CPU::MSR_IA32_MTRR_PHYSBASE0 + i * 2,base | type);
		CPU::setMSR(CPU::MSR_IA32_MTRR_PHYSMASK0 + i * 2,(~(length - 1) & maxphys) | MASK_VALID);
		return 0;
	}
	return -ENOSPC;
}

void MTRR::print(OStream &os) {
	static const char *types[] = {
		/* 0 */	"UNCACHEABLE",
		/* 1 */ "WRITE_COMBINING",
		/* 2 */ "?",
		/* 3 */ "?",
		/* 4 */ "WRITE_THROUGH",
		/* 5 */ "WRITE_PROTECTED",
		/* 6 */ "WRITE_BACK",
	};

	if(!_cap) {
		os.writef("MTRRs not supported\n");
		return;
	}

	os.writef("Phys. address bits: %u\n",_addrbits);
	os.writef("Capabilities:\n");
	os.writef("  VCNT : %u\n",_cap & CAP_VCNT);
	os.writef("  Fixed: %d\n",!!(_cap & CAP_FIXED));
	os.writef("  WC   : %d\n",!!(_cap & CAP_WC));
	os.writef("  SMRR : %d\n",!!(_cap & CAP_SMRR));

	uint64_t def = CPU::getMSR(CPU::MSR_IA32_MTRR_DEF_TYPE);
	os.writef("Default:\n");
	os.writef("  E    : %d\n",!!(def & DEF_E));
	os.writef("  FE   : %d\n",!!(def & DEF_FE));
	os.writef("  Type : %s\n",types[def & DEF_TYPE]);

	os.writef("MTRRs:\n");
	uint count = _cap & CAP_VCNT;
	for(uint i = 0; i < count; ++i) {
		uint64_t base = CPU::getMSR(CPU::MSR_IA32_MTRR_PHYSBASE0 + i * 2);
		uint64_t mask = CPU::getMSR(CPU::MSR_IA32_MTRR_PHYSMASK0 + i * 2);
		if(mask & MASK_VALID)
			os.writef("%u: %016Lx %016Lx : %s\n",i,base,mask,types[base & BASE_TYPE]);
	}
}
