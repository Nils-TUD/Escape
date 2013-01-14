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

#pragma once

#include <esc/common.h>

#define VBEERR_UNSUPPORTED		-10000
#define VBEERR_GETINFO_FAILED	-10001

/* SuperVGA mode information block */
typedef struct {
	uint16_t modeAttributes;		/* Mode attributes                 */
	uint8_t winAAttributes;			/* Window A attributes             */
	uint8_t winBAttributes;			/* Window B attributes             */
	uint16_t winGranularity;		/* Window granularity in k         */
	uint16_t winSize;				/* Window size in k                */
	uint16_t winASegment;			/* Window A segment                */
	uint16_t winBSegment;			/* Window B segment                */
	void (*winFuncPtr)(void);		/* Pointer to window function      */
	uint16_t bytesPerScanLine;		/* Bytes per scanline              */
	uint16_t xResolution;			/* Horizontal resolution           */
	uint16_t yResolution;			/* Vertical resolution             */
	uint8_t xCharSize;				/* Character cell width            */
	uint8_t yCharSize;				/* Character cell height           */
	uint8_t numberOfPlanes;			/* Number of memory planes         */
	uint8_t bitsPerPixel;			/* Bits per pixel                  */
	uint8_t numberOfBanks;			/* Number of CGA style banks       */
	uint8_t memoryModel;			/* Memory model type               */
	uint8_t bankSize;				/* Size of CGA style banks         */
	uint8_t numberOfImagePages;		/* Number of images pages          */
	uint8_t : 8;					/* Reserved                        */
	uint8_t redMaskSize;			/* Size of direct color red mask   */
	uint8_t redFieldPosition;		/* Bit posn of lsb of red mask     */
	uint8_t greenMaskSize;			/* Size of direct color green mask */
	uint8_t greenFieldPosition;		/* Bit posn of lsb of green mask   */
	uint8_t blueMaskSize;			/* Size of direct color blue mask  */
	uint8_t blueFieldPosition;		/* Bit posn of lsb of blue mask    */
	uint8_t rsvdMaskSize;			/* Size of direct color res mask   */
	uint8_t rsvdFieldPosition;		/* Bit posn of lsb of res mask     */
	uint8_t directColorModeInfo;	/* Direct color mode attributes    */
	uint32_t physBasePtr;			/* Physical address for flat memory frame buffer (VBE 2.0) */
	uint16_t modeNo;				/* BEWARE: Used in this driver, reserved by VBE! */
	uint8_t reserved[216];			/* Pad to 256 byte block size      */
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes VBE
 */
void vbe_init(void);

/**
 * @param mode the mode-number
 * @return information about the given mode (or NULL if not found)
 */
sVbeModeInfo *vbe_getModeInfo(uint mode);

/**
 * Allocates an array of sVTMode-objects and fills them with the supported modes.
 *
 * @param count will be set to the number of found modes
 * @return the array
 */
sVTMode *vbe_collectModes(size_t *count);

/**
 * Free's the previously via vbe_collectModes allocated mode.
 *
 * @param modes the modes
 */
void vbe_freeModes(sVTMode *modes);

/**
 * Tries to find the most suitable mode for the given settings
 *
 * @param resX the x-resolution
 * @param resY the y-resolution
 * @param bpp bits per pixel
 * @return the mode or 0 if no matching mode found
 */
uint vbe_findMode(uint resX,uint resY,uint bpp);

/**
 * @return the current mode-number
 */
uint vbe_getMode(void);

/**
 * Sets the given mode
 *
 * @param mode the mode-number
 * @return 0 on success
 */
int vbe_setMode(uint mode);

#ifdef __cplusplus
}
#endif
