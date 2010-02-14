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
#include <string.h>
#include "vbe.h"

/* This is mostly borrowed from doc/vbecore.pdf */

#define VM86_ADDR_RM2PM(addr)				((((u32)(addr) >> 16) << 4) | ((u32)(addr) & 0xFFFF))
#define VM86_CHG_AREA(addr,rmAddr,pmAddr)	((VM86_ADDR_RM2PM(addr) - (u32)(rmAddr)) + (u32)(pmAddr))

#define VM86_DATA_ADDR						0x80000

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

static void vbe_dbg_printMode(sVbeModeInfo *mode,u16 no);

static sVbeInfo vbeInfo;

s32 vbe_loadInfo(void) {
	s32 res;
	sVM86Regs regs;
	sVM86Memarea area;
	regs.ax = 0x4F00;
	regs.es = VM86_SEG(&vbeInfo,VM86_DATA_ADDR);
	regs.di = VM86_OFF(&vbeInfo,VM86_DATA_ADDR);
	area.dst = VM86_DATA_ADDR;
	area.src = &vbeInfo;
	area.size = sizeof(sVbeInfo);
	memcpy(vbeInfo.signature,"VBE2",4);
	if((res = vm86int(0x10,&regs,&area,1)) < 0)
		return res;
	if((regs.ax & 0x00FF) != 0x4F)
		return VBEERR_UNSUPPORTED;
	if((regs.ax & 0xFF00) != 0)
		return VBEERR_GETINFO_FAILED;
	vbeInfo.videoModesPtr = (u16*)VM86_CHG_AREA(vbeInfo.videoModesPtr,VM86_DATA_ADDR,&vbeInfo);
	vbeInfo.oemString = (char*)VM86_CHG_AREA(vbeInfo.oemString,VM86_DATA_ADDR,&vbeInfo);
	return 0;
}

bool vbe_getModeInfo(sVbeModeInfo *info,u16 mode) {
	sVM86Regs regs;
	sVM86Memarea area;
	/* Ignore non-VBE modes */
	if(mode < 0x100)
		return false;
	regs.ax = 0x4F01;
	regs.cx = mode;
	regs.es = VM86_SEG(info,VM86_DATA_ADDR);
	regs.di = VM86_OFF(info,VM86_DATA_ADDR);
	area.dst = VM86_DATA_ADDR;
	area.src = info;
	area.size = sizeof(sVbeModeInfo);
	if(vm86int(0x10,&regs,&area,1) < 0)
		return false;
	if(regs.ax != 0x4F)
		return false;
	return (info->modeAttributes & MODE_SUPPORTED) && info->memoryModel == memPK &&
		info->bitsPerPixel == 8 && info->numberOfPlanes == 1;
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
	regs.bx = mode;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		return false;
	return true;
}

void vbe_printModes(void) {
	u16 *p;
	sVbeModeInfo mode;
	if(vbe_loadInfo() < 0) {
		printe("No VESA VBE detected\n");
		return;
	}

	tFile *f = fopen("/system/devices/vbe","w");
	fprintf(f,"VESA VBE Version %d.%d detected (%x)\n",vbeInfo.version >> 8,
			vbeInfo.version & 0xF,vbeInfo.oemString);
	fprintf(f,"Capabilities: 0x%x\n",vbeInfo.capabilities);
	fprintf(f,"Signature: %c%c%c%c\n",vbeInfo.signature[0],
			vbeInfo.signature[1],vbeInfo.signature[2],vbeInfo.signature[3]);
	fprintf(f,"totalMemory: %d KiB\n",vbeInfo.totalMemory * 64);
	fprintf(f,"videoModes: 0x%x\n",vbeInfo.videoModesPtr);
	fprintf(f,"\n");
	fprintf(f,"Available 256 color video modes:\n");
	for(p = (u16*)vbeInfo.videoModesPtr; *p != (u16)-1; p++) {
		if(vbe_getModeInfo(&mode,*p)) {
			fprintf(f,"    %4d x %4d %d bits per pixel\n",mode.xResolution,mode.yResolution,
					mode.bitsPerPixel);
		}
	}
	fclose(f);
}

static void vbe_dbg_printMode(sVbeModeInfo *mode,u16 no) {
	debugf("Mode %x:\n",no);
	debugf("	attributes = %x\n",mode->modeAttributes);
	debugf("	resolution = %ux%u\n",mode->xResolution,mode->yResolution);
	debugf("	charSize = %ux%u\n",mode->xCharSize,mode->yCharSize);
	debugf("	planes = %u\n",mode->numberOfPlanes);
	debugf("	bpp = %u\n",mode->bitsPerPixel);
	debugf("	memmodel = %u\n",mode->memoryModel);
}

#if 0
/* Set new read/write bank. We must set both Window A and Window B, as
  * many VBE's have these set as separately available read and write
  * windows. We also use a simple (but very effective) optimization of
  * checking if the requested bank is currently active.
  */
void setBank(int bank) {
     union REGS in,out;
     if (bank == curBank) return;
     curBank = bank;
     bank <<= bankShift;
#ifdef DIRECT_BANKING
     setbxdx(0,bank);
     bankSwitch();
     setbxdx(1,bank);
     bankSwitch();
#else
     in.x.ax = 0x4F05; in.x.bx = 0;
     int86(0x10, &in, &out);
     in.x.ax = 0x4F05; in.x.bx = 1;
     int86(0x10, &in, &out);
#endif
}

/* Plot a pixel at location (x,y) in specified color (8 bit modes only) */
void putPixel(int x,int y,int color)
{
    long addr = (long)y * bytesperline + x;
    setBank((int)(addr >> 16));
    *(screenPtr + (addr & 0xFFFF)) = (char)color;
}

sVM86Regs regs;
sVM86Memarea area;
malloc(12);
sVbeInfo *vib = (sVbeInfo*)calloc(512,1);
memcpy(vib->signature,"VBE2",4);
regs.ax = 0x4F00;
regs.es = VM86_SEG(vib,0x80000);
regs.di = VM86_OFF(vib,0x80000);
area.dst = 0x80000;
area.src = vib;
area.size = 512;
if(vm86int(0x10,&regs,&area,1) < 0)
	printe("vm86int failed");

/* Check for errors */
if((regs.ax & 0x00FF) != 0x4F) {
	printe("vbe: VBE is not supported");
} else if((regs.ax & 0xFF00) != 0) {
	printe("vbe: call failed with code 0x%x", regs.ax & 0xFFFF);
}

debugf("vbe: vbe presence was detected:\n");
debugf("          signature: '%s'\n", vib->signature);
debugf("            version: %x\n", vib->version);
debugf("       capabilities: %x\n", vib->capabilities);
debugf(" video mode pointer: 0x%x\n", vib->videoModes);
debugf("       total memory: %uKB\n", vib->totalMemory * 64);
if(vib->version >= 0x0200)
	debugf("   OEM software rev: %x\n", vib->oemString);
#endif
