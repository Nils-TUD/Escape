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

#ifndef VBE_H_
#define VBE_H_

#include <esc/common.h>

#define VBEERR_UNSUPPORTED		-10000
#define VBEERR_GETINFO_FAILED	-10001

/* SuperVGA mode information block */
typedef struct {
	u16 modeAttributes;			/* Mode attributes                 */
	u8 winAAttributes;			/* Window A attributes             */
	u8 winBAttributes;			/* Window B attributes             */
	u16 winGranularity;			/* Window granularity in k         */
	u16 winSize;				/* Window size in k                */
	u16 winASegment;			/* Window A segment                */
	u16 winBSegment;			/* Window B segment                */
	void (*winFuncPtr)(void);	/* Pointer to window function      */
	u16 bytesPerScanLine;		/* Bytes per scanline              */
	u16 xResolution;			/* Horizontal resolution           */
	u16 yResolution;			/* Vertical resolution             */
	u8 xCharSize;				/* Character cell width            */
	u8 yCharSize;				/* Character cell height           */
	u8 numberOfPlanes;			/* Number of memory planes         */
	u8 bitsPerPixel;			/* Bits per pixel                  */
	u8 numberOfBanks;			/* Number of CGA style banks       */
	u8 memoryModel;				/* Memory model type               */
	u8 bankSize;				/* Size of CGA style banks         */
	u8 numberOfImagePages;		/* Number of images pages          */
	u8 : 8;						/* Reserved                        */
	u8 redMaskSize;				/* Size of direct color red mask   */
	u8 redFieldPosition;		/* Bit posn of lsb of red mask     */
	u8 greenMaskSize;			/* Size of direct color green mask */
	u8 greenFieldPosition;		/* Bit posn of lsb of green mask   */
	u8 blueMaskSize;			/* Size of direct color blue mask  */
	u8 blueFieldPosition;		/* Bit posn of lsb of blue mask    */
	u8 rsvdMaskSize;			/* Size of direct color res mask   */
	u8 rsvdFieldPosition;		/* Bit posn of lsb of res mask     */
	u8 directColorModeInfo;		/* Direct color mode attributes    */
	u32 physBasePtr;			/* Physical address for flat memory frame buffer (VBE 2.0) */
	u16 modeNo;					/* BEWARE: Used in this driver, reserved by VBE! */
	u8 reserved[216];			/* Pad to 256 byte block size      */
} A_PACKED sVbeModeInfo;

#define MODE_SUPPORTED				(1 << 0)	/* Wether the mode is supported */
#define MODE_TTYOUT_FUNCS_SUP		(1 << 2)	/* TTY Output functions supported by BIOS */
#define MODE_COLOR_MODE				(1 << 3)	/* monochrome / color mode */
#define MODE_GRAPHICS_MODE			(1 << 4)	/* Text-mode / graphics mode */
#define MODE_VGA_INCOMPATIBLE		(1 << 5)	/* VGA compatible mode */
#define MODE_VGA_WINMEM_NOTAVAIL	(1 << 6)	/* VGA compatible windowed memory mode is available */
#define MODE_LIN_FRAME_BUFFER		(1 << 7)	/* Linear frame buffer mode is available */
#define MODE_DOUBLE_SCANMODE		(1 << 8)	/* Double scan mode is available */
#define MODE_INTERLACED				(1 << 9)	/* Interlaced mode is available */
#define MODE_HW_TRIPLE_BUFFER		(1 << 10)	/* Hardware triple buffering support */
#define MODE_HW_STEREOSCOPIC_DISP	(1 << 11)	/* Hardware stereoscopic display support */
#define MODE_DUAL_DISPL_START_ADDR	(1 << 12)	/* Dual display start address support */

typedef enum {
	memPL	= 3,				/* Planar memory model */
	memPK	= 4,				/* Packed pixel memory model     */
	memRGB	= 6,				/* Direct color RGB memory model */
	memYUV	= 7,				/* Direct color YUV memory model */
} eMemModel;

/**
 * Initializes VBE
 */
void vbe_init(void);

/**
 * @param mode the mode-number
 * @return information about the given mode (or NULL if not found)
 */
sVbeModeInfo *vbe_getModeInfo(u16 mode);

/**
 * Tries to find the most suitable mode for the given settings
 *
 * @param resX the x-resolution
 * @param resY the y-resolution
 * @param bpp bits per pixel
 * @return the mode or 0 if no matching mode found
 */
u16 vbe_findMode(u16 resX,u16 resY,u16 bpp);

/**
 * @return the current mode-number
 */
u16 vbe_getMode(void);

/**
 * Sets the given mode
 *
 * @param mode the mode-number
 * @return true on success
 */
bool vbe_setMode(u16 mode);

#endif /* VBE_H_ */
