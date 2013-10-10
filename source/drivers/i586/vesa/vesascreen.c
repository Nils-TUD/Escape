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

#include <esc/common.h>
#include <esc/mem.h>
#include <esc/sllist.h>
#include <vbe/vbe.h>
#include <stdio.h>
#include <stdlib.h>

#include "vesascreen.h"
#include "vesatext.h"
#include "font.h"
#include "font.inc"

static int vesascr_initWhOnBl(sVESAScreen *scr);

static sSLList *screens;

static int vesascr_initWhOnBl(sVESAScreen *scr) {
	scr->whOnBlCache = (uint8_t*)malloc((FONT_WIDTH + PAD * 2) * (FONT_HEIGHT + PAD * 2) *
			(scr->mode->bitsPerPixel / 8) * FONT_COUNT);
	if(scr->whOnBlCache == NULL)
		return -ENOMEM;

	uint8_t *cc = scr->whOnBlCache;
	uint8_t *white = vesat_getColor(WHITE);
	uint8_t *black = vesat_getColor(BLACK);
	for(size_t i = 0; i < FONT_COUNT; i++) {
		for(int y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
			for(int x = 0; x < FONT_WIDTH + PAD * 2; x++) {
				if(y >= PAD && y < FONT_HEIGHT + PAD && x >= PAD && x < FONT_WIDTH + PAD &&
						PIXEL_SET(i,x - PAD,y - PAD)) {
					cc = vesat_setPixel(scr,cc,white);
				}
				else
					cc = vesat_setPixel(scr,cc,black);
			}
		}
	}
	return 0;
}

sVESAScreen *vesascr_request(sVbeModeInfo *minfo) {
	if(screens == NULL && (screens = sll_create()) == NULL)
		return NULL;

	/* is there already a screen for that mode? */
	sVESAScreen *scr = NULL;
	for(sSLNode *n = sll_begin(screens); n != NULL; n = n->next) {
		scr = (sVESAScreen*)n->data;
		if(scr->mode->modeNo == minfo->modeNo) {
			scr->refs++;
			return scr;
		}
	}

	/* ok, create a new one */
	scr = (sVESAScreen*)malloc(sizeof(sVESAScreen));
	scr->refs = 1;
	scr->cols = minfo->xResolution / (FONT_WIDTH + PAD * 2);
	/* leave at least one pixel free for the cursor */
	scr->rows = (minfo->yResolution - 1) / (FONT_HEIGHT + PAD * 2);
	scr->mode = minfo;
	if(!sll_append(screens,scr)) {
		free(scr);
		return NULL;
	}

	/* map framebuffer */
	size_t size = minfo->xResolution * minfo->yResolution * (minfo->bitsPerPixel / 8);
	uintptr_t phys = minfo->physBasePtr;
	scr->frmbuf = regaddphys(&phys,size,0);
	if(scr->frmbuf == NULL) {
		sll_removeFirstWith(screens,scr);
		free(scr);
		return NULL;
	}

	/* init white-on-black cache */
	vesascr_initWhOnBl(scr);

	/* alloc content */
	scr->content = (uint8_t*)malloc(scr->cols * scr->rows * 2);
	if(scr->content == NULL) {
		munmap(scr->frmbuf);
		sll_removeFirstWith(screens,scr);
		free(scr);
		return NULL;
	}
	return scr;
}

void vesascr_reset(sVESAScreen *scr,int type) {
	scr->lastCol = scr->cols;
	scr->lastRow = scr->rows;
	memclear(scr->frmbuf,
		scr->mode->xResolution * scr->mode->yResolution * (scr->mode->bitsPerPixel / 8));
	if(type == VID_MODE_TYPE_TUI) {
		for(int y = 0; y < scr->rows; y++) {
			for(int x = 0; x < scr->cols; x++) {
				scr->content[y * scr->cols * 2 + x * 2] = ' ';
				scr->content[y * scr->cols * 2 + x * 2 + 1] = 0x07;
			}
		}
	}
}

void vesascr_release(sVESAScreen *scr) {
	if(--scr->refs == 0) {
		free(scr->whOnBlCache);
		free(scr->content);
		munmap(scr->frmbuf);
		sll_removeFirstWith(screens,scr);
		free(scr);
	}
}
