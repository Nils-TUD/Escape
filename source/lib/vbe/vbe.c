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

#include <esc/common.h>
#include <esc/arch/i586/vm86.h>
#include <esc/debug.h>
#include <esc/sllist.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <vbe/vbe.h>

/* This is mostly borrowed from doc/vbecore.pdf */

#define VM86_DATA_ADDR						0x80000
#define MAX_MODE_COUNT						256
/* for setting a mode: (VBE v2.0+) use linear (flat) frame buffer */
#define VBE_MODE_SET_LFB					0x4000

/* SuperVGA information block */
typedef struct {
	char signature[4];			/* 'VESA' 4 byte signature      */
	uint16_t version;			/* VBE version number           */
	char *oemString;			/* Pointer to OEM string        */
	uint32_t capabilities;		/* Capabilities of video card   */
	uint16_t *videoModesPtr;	/* Pointer to supported modes   */
	uint16_t totalMemory;		/* Number of 64kb memory blocks */
	uint8_t reserved[236];		/* Pad to 256 byte block size   */
} A_PACKED sVbeInfo;

static bool vbe_isSupported(sVbeModeInfo *info);
static int vbe_loadInfo(void);
static bool vbe_loadModeInfo(sVbeModeInfo *info,uint mode);
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

sScreenMode *vbe_getModeInfo(int modeno) {
	for(sSLNode *n = sll_begin(modes); n != NULL; n = n->next) {
		sScreenMode *mode = (sScreenMode*)n->data;
		if(mode->id == modeno)
			return mode;
	}
	return NULL;
}

sScreenMode *vbe_collectModes(size_t n,size_t *count) {
	sSLNode *node;
	sScreenMode *res = NULL;
	ssize_t max = n ? (ssize_t)MIN(n,sll_length(modes)) : -1;
	if(n)
		res = (sScreenMode*)malloc(sizeof(sScreenMode) * max);
	*count = 0;
	for(node = sll_begin(modes); (max == -1 || max > 0) && node != NULL ; node = node->next) {
		sScreenMode *info = (sScreenMode*)node->data;
		if(n) {
			memcpy(res + *count,info,sizeof(sScreenMode));
			max--;
		}
		(*count)++;
	}
	return res;
}

void vbe_freeModes(sScreenMode *m) {
	free(m);
}

uint vbe_getMode(void) {
	sVM86Regs regs;
	regs.ax = 0x4F03;
	if(vm86int(0x10,&regs,NULL) < 0)
		return 0;
	return regs.bx;
}

int vbe_setMode(uint mode) {
	sVM86Regs regs;
	regs.ax = 0x4F02;
	regs.bx = mode | VBE_MODE_SET_LFB;
	return vm86int(0x10,&regs,NULL);
}

static bool vbe_isSupported(sVbeModeInfo *info) {
	/* skip unsupported modes */
	if(!(info->modeAttributes & MODE_COLOR_MODE))
		return false;
	if(!(info->modeAttributes & MODE_GRAPHICS_MODE))
		return false;
	if(!(info->modeAttributes & MODE_LIN_FRAME_BUFFER))
		return false;
	if(info->memoryModel != memRGB/* && info->memoryModel != memPK*/)
		return false;
	if(info->bitsPerPixel != 16 && info->bitsPerPixel != 24 && info->bitsPerPixel != 32)
		return false;
	return true;
}

static int vbe_loadInfo(void) {
	int res;
	sVM86Regs regs;
	sVM86Memarea area;
	sVM86AreaPtr ptr;
	regs.ax = 0x4F00;
	regs.es = VM86_SEG(VM86_DATA_ADDR);
	regs.di = VM86_OFF(VM86_DATA_ADDR);
	area.dst = VM86_DATA_ADDR;
	area.src = &vbeInfo;
	area.size = sizeof(sVbeInfo);
	area.ptr = &ptr;
	area.ptrCount = 1;
	ptr.offset = offsetof(sVbeInfo,videoModesPtr);
	ptr.result = (uintptr_t)malloc(MAX_MODE_COUNT * sizeof(uint));
	if(ptr.result == 0)
		return VBEERR_GETINFO_FAILED;
	ptr.size = MAX_MODE_COUNT * sizeof(uint);
	memcpy(vbeInfo.signature,"VBE2",4);
	if((res = vm86int(0x10,&regs,&area)) < 0)
		return res;
	if((regs.ax & 0x00FF) != 0x4F)
		return VBEERR_UNSUPPORTED;
	if((regs.ax & 0xFF00) != 0)
		return VBEERR_GETINFO_FAILED;
	vbeInfo.videoModesPtr = (uint16_t*)ptr.result;
	/* TODO */
	vbeInfo.oemString = 0;
	return 0;
}

static bool vbe_loadModeInfo(sVbeModeInfo *info,uint mode) {
	sVM86Regs regs;
	sVM86Memarea area;
	/* Ignore non-VBE modes */
	if(mode < 0x100)
		return false;
	regs.ax = 0x4F01;
	regs.cx = mode;
	regs.es = VM86_SEG(VM86_DATA_ADDR);
	regs.di = VM86_OFF(VM86_DATA_ADDR);
	area.dst = VM86_DATA_ADDR;
	area.src = info;
	area.size = sizeof(sVbeModeInfo);
	area.ptrCount = 0;
	if(vm86int(0x10,&regs,&area) < 0)
		return false;
	if(regs.ax != 0x4F)
		return false;
	info->modeNo = mode;
	/* physBasePtr is only present (!= 0) with VBE 2.0 */
	return (info->modeAttributes & MODE_SUPPORTED) && info->physBasePtr != 0;
}

static void vbe_detectModes(void) {
	uint16_t *p;
	size_t i;
	sVbeModeInfo mode;
	FILE *f = fopen("/system/devices/vbe","w");
	if(!f)
		printe("Unable to open /system/devices/vbe for writing");
	else {
		fprintf(f,"VESA VBE Version %d.%d detected (%p)\n",vbeInfo.version >> 8,
				vbeInfo.version & 0xF,(void*)vbeInfo.oemString);
		fprintf(f,"Capabilities: %#x\n",vbeInfo.capabilities);
		fprintf(f,"Signature: %c%c%c%c\n",vbeInfo.signature[0],
				vbeInfo.signature[1],vbeInfo.signature[2],vbeInfo.signature[3]);
		fprintf(f,"VideoMemory: %d KiB\n",vbeInfo.totalMemory * 64);
		fprintf(f,"VideoModes: %p\n",(void*)vbeInfo.videoModesPtr);
		fprintf(f,"\n");
		fprintf(f,"Available video modes:\n");
	}
	p = (uint16_t*)vbeInfo.videoModesPtr;
	for(i = 0; i < MAX_MODE_COUNT && *p != (uint16_t)-1; i++, p++) {
		if(vbe_loadModeInfo(&mode,*p)) {
			if(f) {
				fprintf(f,"  %5d: %4d x %4d %2d bpp, %d planes, %s, %s %s (%x), @%p\n",mode.modeNo,
						mode.xResolution,mode.yResolution,
						mode.bitsPerPixel,mode.numberOfPlanes,
						(mode.modeAttributes & MODE_GRAPHICS_MODE) ? "graphics" : "    text",
						(mode.modeAttributes & MODE_COLOR_MODE) ? "color" : " mono",
						mode.memoryModel == memPL ? " plane" :
						mode.memoryModel == memPK ? "packed" :
						mode.memoryModel == memRGB ? "   RGB" :
						mode.memoryModel == memYUV ? "   YUV" : "??????",
						mode.modeAttributes,
						(void*)mode.physBasePtr);
			}

			if(vbe_isSupported(&mode)) {
				sScreenMode *scrMode = (sScreenMode*)malloc(sizeof(sScreenMode));
				if(scrMode != NULL) {
					scrMode->id = mode.modeNo;
					scrMode->width = mode.xResolution;
					scrMode->height = mode.yResolution;
					scrMode->bitsPerPixel = mode.bitsPerPixel;
					scrMode->redMaskSize = mode.redMaskSize;
					scrMode->redFieldPosition = mode.redFieldPosition;
					scrMode->greenMaskSize = mode.greenMaskSize;
					scrMode->greenFieldPosition = mode.greenFieldPosition;
					scrMode->blueMaskSize = mode.blueMaskSize;
					scrMode->blueFieldPosition = mode.blueFieldPosition;
					scrMode->physaddr = mode.physBasePtr;
					scrMode->tuiHeaderSize = 0;
					scrMode->guiHeaderSize = 0;
					scrMode->mode = VID_MODE_GRAPHICAL;
					scrMode->type = VID_MODE_TYPE_GUI | VID_MODE_TYPE_TUI;
					sll_append(modes,scrMode);
				}
			}
		}
	}
	if(f)
		fclose(f);
}
