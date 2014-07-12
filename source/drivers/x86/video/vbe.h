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
#include <map>

#include "x86emu/x86emu.h"
#include "pcibus.h"

#define ADDR_OFF(addr)      ((addr) & 0xFFFF)
#define ADDR_SEG(addr)      (((addr) & 0xF0000) >> 4)

class VBE {
	VBE() = delete;

	struct InfoBlock {
		uint32_t tag;
		uint16_t version;
		uint32_t oem_string;
		uint32_t caps;
		uint32_t video_mode_ptr;
		uint16_t memory;
		uint16_t oem_revision;
		uint32_t oem_vendor;
		uint32_t oem_product;
		uint32_t oem_product_rev;
	} A_PACKED;

public:
	struct ModeInfo {
		uint16_t modeAttributes;				/* Mode attributes */
		uint8_t winAAttributes;					/* Window A attributes */
		uint8_t winBAttributes;					/* Window B attributes */
		uint16_t winGranularity;				/* Window granularity in k */
		uint16_t winSize;						/* Window size in k */
		uint16_t winASegment;					/* Window A segment */
		uint16_t winBSegment;					/* Window B segment */
		uint32_t winFuncPtr;					/* Pointer to window function */
		uint16_t bytesPerScanLine;				/* Bytes per scanline */
		uint16_t xResolution;					/* Horizontal resolution */
		uint16_t yResolution;					/* Vertical resolution */
		uint8_t xCharSize;						/* Character cell width */
		uint8_t yCharSize;						/* Character cell height */
		uint8_t numberOfPlanes;					/* Number of memory planes */
		uint8_t bitsPerPixel;					/* Bits per pixel */
		uint8_t numberOfBanks;					/* Number of CGA style banks */
		uint8_t memoryModel;					/* Memory model type */
		uint8_t bankSize;						/* Size of CGA style banks */
		uint8_t numberOfImagePages;				/* Number of images pages */
		uint8_t : 8;							/* Reserved */
		uint8_t redMaskSize;					/* Size of direct color red mask */
		uint8_t redFieldPosition;				/* Bit posn of lsb of red mask */
		uint8_t greenMaskSize;					/* Size of direct color green mask */
		uint8_t greenFieldPosition;				/* Bit posn of lsb of green mask */
		uint8_t blueMaskSize;					/* Size of direct color blue mask */
		uint8_t blueFieldPosition;				/* Bit posn of lsb of blue mask */
		uint8_t rsvdMaskSize;					/* Size of direct color res mask */
		uint8_t rsvdFieldPosition;				/* Bit posn of lsb of res mask */
		uint8_t directColorModeInfo;			/* Direct color mode attributes */
		uint32_t physBasePtr;					/* Physical address for flat memory frame buffer (VBE 2.0) */
		uint16_t modeNo;						/* BEWARE: Used in this driver, reserved by VBE! */
		uint8_t reserved[216];					/* Pad to 256 byte block size */
	} A_PACKED;

	enum {
		MODE_SUPPORTED				= 1 << 0,	/* Wether the mode is supported */
		MODE_TTYOUT_FUNCS_SUP		= 1 << 2,	/* TTY Output functions supported by BIOS */
		MODE_COLOR_MODE				= 1 << 3,	/* monochrome / color mode */
		MODE_GRAPHICS_MODE			= 1 << 4,	/* Text-mode / graphics mode */
		MODE_VGA_INCOMPATIBLE		= 1 << 5,	/* VGA compatible mode */
		MODE_VGA_WINMEM_NOTAVAIL	= 1 << 6,	/* VGA compatible windowed memory mode is available */
		MODE_LIN_FRAME_BUFFER		= 1 << 7,	/* Linear frame buffer mode is available */
		MODE_DOUBLE_SCANMODE		= 1 << 8,	/* Double scan mode is available */
		MODE_INTERLACED				= 1 << 9,	/* Interlaced mode is available */
		MODE_HW_TRIPLE_BUFFER		= 1 << 10,	/* Hardware triple buffering support */
		MODE_HW_STEREOSCOPIC_DISP	= 1 << 11,	/* Hardware stereoscopic display support */
		MODE_DUAL_DISPL_START_ADDR	= 1 << 12,	/* Dual display start address support */
	};

private:
	enum {
		VBE_CONTROL_FUNC			= 0x4F00,
		VBE_INFO_FUNC				= 0x4F01,
		VBE_MODE_FUNC				= 0x4F02,
		VBE_GMODE_FUNC				= 0x4F03,
	};

	enum {
		MODE_SET_LFB				= 0x4000,	/* linear frame buffer (VBE v2.0+) */
		MODE_SET_PRESERVE			= 0x8000,	/* don't clear the screen */
	};

	static const uint32_t TAG_VBE2	= 0x32454256;

	static const uintptr_t ENTRY	= 0x7000;
	static const uintptr_t DATA		= 0x4000;
	static const uintptr_t STACK	= 0x7DF8;
	static const uintptr_t ES_SEG0	= 0x80000;
	static const uintptr_t ES_SEG1	= 0x82000;

public:
	typedef std::map<int, ModeInfo *> modes_map;
	typedef modes_map::iterator iterator;

	static void init();

	static iterator begin() {
		return _modes.begin();
	}
	static iterator end() {
		return _modes.end();
	}

	static bool getInfo(int mode, ModeInfo &info) {
		modes_map::iterator it = _modes.find(mode);
		if(it == _modes.end())
			return false;
		info = *it->second;
		return true;
	}
	static void setMode(int mode) {
		x86emuExec(VBE_MODE_FUNC,mode | MODE_SET_PRESERVE | MODE_SET_LFB,0,0,ADDR_SEG(ES_SEG0));
	}

private:
	template<typename T>
	static T vbeToMem(unsigned ptr) {
		return reinterpret_cast<T>(_mem + (ptr & 0xffff) + ((ptr >> 12) & 0xffff0));
	}
	static void addMode(unsigned short mode,unsigned seg);
	static uint16_t x86emuExec(uint16_t eax,uint16_t ebx,uint16_t ecx,uint16_t edi,uint16_t es);

	template <typename T>
	static T X86API inx(X86EMU_pioAddr addr);
	template <typename T>
	static void X86API outx(X86EMU_pioAddr addr,T val);

	static unsigned _version;
	static modes_map _modes;
	static char *_mem;
	static PCIBus _pcibus;
	static X86EMU_pioFuncs _portFuncs;
};
