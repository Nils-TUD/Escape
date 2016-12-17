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

#include <sys/arch/x86/ports.h>
#include <sys/common.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include "x86emu/x86emu.h"
#include "pcibus.h"
#include "vbe.h"

static bool debugPrints = false;
static bool debugPorts = false;

unsigned VBE::_version;
VBE::modes_map VBE::_modes;
char *VBE::_mem;
PCIBus VBE::_pcibus;
X86EMU_pioFuncs VBE::_portFuncs = {
	&VBE::inx,  &VBE::inx,  &VBE::inx,
	&VBE::outx, &VBE::outx, &VBE::outx
};

template <typename T>
T X86API VBE::inx(X86EMU_pioAddr addr) {
	T ret = 0;
	if(!_pcibus.read(addr,&ret)) {
		switch(sizeof(T)) {
			case 1:
				ret = inbyte(addr);
				break;
			case 2:
				ret = inword(addr);
				break;
			default:
				ret = indword(addr);
				break;
		}
	}

	if(debugPorts)
		print("port [%04x,%04zx) -> 0x%x",(unsigned short)addr,addr + sizeof(T),ret);
	return ret;
}

template <typename T>
void X86API VBE::outx(X86EMU_pioAddr addr,T val) {
	unsigned v = val;
	if(!_pcibus.write(addr,val)) {
		switch(sizeof(T)) {
			case 1:
				outbyte(addr,val);
				break;
			case 2:
				outword(addr,val);
				break;
			default:
				outdword(addr,val);
				break;
		}
	}

	if(debugPorts)
		print("port [%04x,%04zx) <- 0x%x",(unsigned short)addr,addr + sizeof(T),v);
}

void VBE::init() {
	// map BIOS memory (first megabyte)
	uintptr_t phys = 0;
	_mem = reinterpret_cast<char*>(mmapphys(&phys,1 << 20,0,MAP_PHYS_MAP));
	if(!_mem)
		error("Unable to map BIOS memory");

	// announce our port handler functions
	X86EMU_setupPioFuncs(&_portFuncs);

	// it's enough to just assign the memory location. we don't need to implement the reads/writes
	// here ourself
	M.mem_base = (uintptr_t)_mem;
	M.mem_size = 1 << 20;
	// set debug flags
	M.x86.debug = 0;

	// build startup and finish code
	uint32_t code = 0;
	code  = 0xcd;			/* int */
	code |= 0x10 << 8;		/* 10h  */
	code |= 0xf4 << 16;		/* ret */
	memcpy(_mem + ENTRY,&code,sizeof(code));

	// TODO as it seems, every BIOS accesses different I/O ports in the VBE functions (sometimes
	// they are important, sometimes ignoring them is fine). since I don't see any way to figure out
	// in advance which ports are necessary, we just request all.
	if(reqports(0x0,0xFFFF) < 0)
		error("Unable to request ports");

	// get info block
	InfoBlock *p = reinterpret_cast<InfoBlock*>(_mem + ES_SEG0);
	p->tag = VBE::TAG_VBE2;
	x86emuExec(VBE_CONTROL_FUNC,0,0,addrToSegOffset(ES_SEG0),addrToSeg(ES_SEG0));
	if(p->version < 0x200)
		error("VBE version %d too old ( >= 2.0 required)",p->version);
	// save version for later usage
	_version = p->version;

	print("Found VBE:");
	print("   Version: %#x",p->version);
	print("   Tag: %#x",p->tag);
	print("   Memory size: %#x",p->memory << 16);
	print("   OEM: %s",vbeToMem<char*>(p->oem_string));
	print("   Vendor: %s",vbeToMem<char*>(p->oem_vendor));
	print("   Product: %s",vbeToMem<char*>(p->oem_product));
	print("   Product revision: %s",vbeToMem<char*>(p->oem_product_rev));

	// collect modes
	uint16_t *video_mode_ptr = vbeToMem<uint16_t*>(p->video_mode_ptr);
	for(size_t i = 0; video_mode_ptr[i] != 0xffff; i++) {
		uint16_t mode = vbeToMem<uint16_t*>(p->video_mode_ptr)[i];
		x86emuExec(VBE_INFO_FUNC,0,mode,addrToSegOffset(ES_SEG1),addrToSeg(ES_SEG1));
		addMode(mode,ES_SEG1);
	}
}

void VBE::addMode(unsigned short mode,unsigned seg) {
	static const char *memmodels[] = {
		"Text",
		"CGA",
		"Hercules",
		"Planar",
		"Packed",
		"Nonchain",
		"Direct",
		"YUV"
	};

	ModeInfo *modeinfo = reinterpret_cast<ModeInfo*>(_mem + seg);

	// validate bytes_per_scanline
	if(_version < 0x300 || !modeinfo->bytesPerScanLine)
		modeinfo->bytesPerScanLine = (modeinfo->xResolution * modeinfo->bitsPerPixel / 8);

	ModeInfo *copy = new ModeInfo(*modeinfo);
	_modes[mode] = copy;

	print("Mode %#3d %s %4ux%4ux%2u phys %#08x attr %#x bps %#06x planes %u memmodel %s",
		mode,
		(modeinfo->modeAttributes & 0x80) ? "linear" : "window",
		modeinfo->xResolution,modeinfo->yResolution,modeinfo->bitsPerPixel,
		modeinfo->physBasePtr,
		modeinfo->modeAttributes,
		modeinfo->bytesPerScanLine,
		modeinfo->numberOfPlanes,
		modeinfo->memoryModel < ARRAY_SIZE(memmodels) ? memmodels[modeinfo->memoryModel] : "??");
}

uint16_t VBE::x86emuExec(uint16_t eax,uint16_t ebx,uint16_t ecx,uint16_t edi,uint16_t es) {
	M.x86.R_EAX  = eax;
	M.x86.R_EBX  = ebx;
	M.x86.R_ECX  = ecx;
	M.x86.R_EDI  = edi;
	M.x86.R_IP   = addrToSegOffset(ENTRY);
	M.x86.R_CS   = addrToSeg(ENTRY);
	M.x86.R_DS   = addrToSeg(DATA);
	M.x86.R_ES   = es;
	M.x86.R_SP   = addrToSegOffset(STACK);
	M.x86.R_SS   = addrToSeg(STACK);
	X86EMU_exec();
	return M.x86.R_AX;
}

EXTERN_C void sprintf(char *str,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vsnprintf(str,std::numeric_limits<size_t>::max(),fmt,ap);
	va_end(ap);
}

EXTERN_C void printk(const char *fmt,...) {
	if(debugPrints) {
		va_list ap;
		va_start(ap,fmt);
		vprintf(fmt,ap);
		va_end(ap);
	}
}
