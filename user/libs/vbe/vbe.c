/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/vm86.h>
#include <esc/fileio.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/heap.h>
#include <string.h>
#include <sllist.h>
#include <assert.h>
#include <limits.h>
#include "vbe.h"

/* This is mostly borrowed from doc/vbecore.pdf */

#define VM86_ADDR_RM2PM(addr)				((((u32)(addr) & 0xFFFF0000) >> 12) | ((u32)(addr) & 0xFFFF))
#define VM86_CHG_AREA(addr,rmAddr,pmAddr)	(((u32)(addr) - (u32)(rmAddr)) + (u32)(pmAddr))
#define ABS(x)								((x) > 0 ? (x) : -(x))

#define VM86_DATA_ADDR						0x80000

#define MAX_MODE_COUNT						256

/* for setting a mode: (VBE v2.0+) use linear (flat) frame buffer */
#define VBE_MODE_SET_LFB					0x4000

/* SuperVGA information block */
typedef struct {
	char signature[4];			/* 'VESA' 4 byte signature      */
	u16 version;				/* VBE version number           */
	char *oemString;			/* Pointer to OEM string        */
	u32 capabilities;			/* Capabilities of video card   */
	u16 *videoModesPtr;			/* Pointer to supported modes   */
	u16 totalMemory;			/* Number of 64kb memory blocks */
	u8 reserved[236];			/* Pad to 256 byte block size   */
} A_PACKED sVbeInfo;

static s32 vbe_loadInfo(void);
static bool vbe_loadModeInfo(sVbeModeInfo *info,u16 mode);
static void vbe_detectModes(void);

static sSLList *modes = NULL;
static sVbeInfo vbeInfo;

void vbe_init(void) {
	modes = sll_create();
	if(modes == NULL) {
		printe("Not enough mem for mode-list");
		return;
	}
	if(vbe_loadInfo() < 0) {
		printe("No VESA VBE detected");
		return;
	}

	vbe_detectModes();
}

sVbeModeInfo *vbe_getModeInfo(u16 mode) {
	sSLNode *n;
	sVbeModeInfo *info;
	for(n = sll_begin(modes); n != NULL; n = n->next) {
		info = (sVbeModeInfo*)n->data;
		if(info->modeNo == mode)
			return info;
	}
	return NULL;
}

u16 vbe_findMode(u16 resX,u16 resY,u16 bpp) {
	sSLNode *n;
	sVbeModeInfo *info;
	u16 best;
	u32 pixdiff, bestpixdiff = ABS(320 * 200 - resX * resY);
	u32 depthdiff, bestdepthdiff = 8 >= bpp ? 8 - bpp : (bpp - 8) * 2;
	for(n = sll_begin(modes); n != NULL; n = n->next) {
		info = (sVbeModeInfo*)n->data;
		/* skip unsupported modes */
		if(!(info->modeAttributes & MODE_COLOR_MODE))
			continue;
		if(!(info->modeAttributes & MODE_GRAPHICS_MODE))
			continue;
		if(!(info->modeAttributes & MODE_LIN_FRAME_BUFFER))
			continue;
		if(info->memoryModel != memRGB/* && info->memoryModel != memPK*/)
			continue;
		if(info->bitsPerPixel != 16 && info->bitsPerPixel != 24 && info->bitsPerPixel != 32)
			continue;

		/* exact match? */
		if(info->xResolution == resX && info->yResolution == resY && info->bitsPerPixel == bpp)
			return info->modeNo;

		/* Otherwise, compare to the closest match so far, remember if best */
		pixdiff = ABS(info->xResolution * info->yResolution - resX * resY);
		if(info->bitsPerPixel >= bpp)
			depthdiff = info->bitsPerPixel - bpp;
		else
			depthdiff = (bpp - info->bitsPerPixel) * 2;
		if(bestpixdiff > pixdiff || (bestpixdiff == pixdiff && bestdepthdiff > depthdiff)) {
			best = info->modeNo;
			bestpixdiff = pixdiff;
			bestdepthdiff = depthdiff;
		}
	}
	return best;
}

u16 vbe_getMode(void) {
	sVM86Regs regs;
	regs.ax = 0x4F03;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		return 0;
	return regs.bx;
}

bool vbe_setMode(u16 mode) {
	sVM86Regs regs;
	regs.ax = 0x4F02;
	regs.bx = mode | VBE_MODE_SET_LFB;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		return false;
	return true;
}

static s32 vbe_loadInfo(void) {
	s32 res;
	sVM86Regs regs;
	sVM86Memarea areas[2];
	regs.ax = 0x4F00;
	regs.es = VM86_SEG(&vbeInfo,VM86_DATA_ADDR);
	regs.di = VM86_OFF(&vbeInfo,VM86_DATA_ADDR);
	areas[0].type = VM86_MEM_DIRECT;
	areas[0].data.direct.dst = VM86_DATA_ADDR;
	areas[0].data.direct.src = &vbeInfo;
	areas[0].data.direct.size = sizeof(sVbeInfo);
	areas[1].type = VM86_MEM_PTR;
	areas[1].data.ptr.srcPtr = (void**)&vbeInfo.videoModesPtr;
	areas[1].data.ptr.result = (u32)malloc(MAX_MODE_COUNT * sizeof(u32));
	if(areas[1].data.ptr.result == 0)
		return VBEERR_GETINFO_FAILED;
	areas[1].data.ptr.size = MAX_MODE_COUNT * sizeof(u32);
	memcpy(vbeInfo.signature,"VBE2",4);
	if((res = vm86int(0x10,&regs,areas,2)) < 0)
		return res;
	if((regs.ax & 0x00FF) != 0x4F)
		return VBEERR_UNSUPPORTED;
	if((regs.ax & 0xFF00) != 0)
		return VBEERR_GETINFO_FAILED;
	vbeInfo.videoModesPtr = (u16*)areas[1].data.ptr.result;
	/* TODO */
	vbeInfo.oemString = 0;
	return 0;
}

static bool vbe_loadModeInfo(sVbeModeInfo *info,u16 mode) {
	sVM86Regs regs;
	sVM86Memarea area;
	/* Ignore non-VBE modes */
	if(mode < 0x100)
		return false;
	regs.ax = 0x4F01;
	regs.cx = mode;
	regs.es = VM86_SEG(info,VM86_DATA_ADDR);
	regs.di = VM86_OFF(info,VM86_DATA_ADDR);
	area.type = VM86_MEM_DIRECT;
	area.data.direct.dst = VM86_DATA_ADDR;
	area.data.direct.src = info;
	area.data.direct.size = sizeof(sVbeModeInfo);
	if(vm86int(0x10,&regs,&area,1) < 0)
		return false;
	if(regs.ax != 0x4F)
		return false;
	info->modeNo = mode;
	/* physBasePtr is only present (!= 0) with VBE 2.0 */
	return (info->modeAttributes & MODE_SUPPORTED) && info->physBasePtr != 0;
}

static void vbe_detectModes(void) {
	u16 i,*p;
	sVbeModeInfo mode;
	sVbeModeInfo *modeCpy;
	tFile *f = fopen("/system/devices/vbe","w");
	fprintf(f,"VESA VBE Version %d.%d detected (%x)\n",vbeInfo.version >> 8,
			vbeInfo.version & 0xF,vbeInfo.oemString);
	fprintf(f,"Capabilities: 0x%x\n",vbeInfo.capabilities);
	fprintf(f,"Signature: %c%c%c%c\n",vbeInfo.signature[0],
			vbeInfo.signature[1],vbeInfo.signature[2],vbeInfo.signature[3]);
	fprintf(f,"totalMemory: %d KiB\n",vbeInfo.totalMemory * 64);
	fprintf(f,"videoModes: 0x%x\n",vbeInfo.videoModesPtr);
	fprintf(f,"\n");
	fprintf(f,"Available video modes:\n");
	for(i = 0, p = (u16*)vbeInfo.videoModesPtr; i < MAX_MODE_COUNT && *p != (u16)-1; i++, p++) {
		if(vbe_loadModeInfo(&mode,*p)) {
			fprintf(f,"  %4x: %4d x %4d %2d bpp, %d planes, %s, %s, %s (attr %x) (@%x)\n",mode.modeNo,
					mode.xResolution,mode.yResolution,
					mode.bitsPerPixel,mode.numberOfPlanes,
					(mode.modeAttributes & MODE_GRAPHICS_MODE) ? "graphics" : "    text",
					(mode.modeAttributes & MODE_COLOR_MODE) ? "     color" : "monochrome",
					mode.memoryModel == memPL ? " plane" :
					mode.memoryModel == memPK ? "packed" :
					mode.memoryModel == memRGB ? "   RGB" :
					mode.memoryModel == memYUV ? "   YUV" : "??????",
					mode.modeAttributes,
					mode.physBasePtr);
			modeCpy = (sVbeModeInfo*)malloc(sizeof(sVbeModeInfo));
			if(modeCpy != NULL) {
				memcpy(modeCpy,&mode,sizeof(sVbeModeInfo));
				sll_append(modes,modeCpy);
			}
		}
	}
	fclose(f);
}
