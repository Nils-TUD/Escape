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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/dbg/kb.h>
#include <esc/keycodes.h>

#define KEYBOARD_BASE		0x8006000000000000	/* physical keyboard base address */

#define KEYBOARD_CTRL		0			/* keyboard control register */
#define KEYBOARD_DATA		1			/* keyboard data register */

#define KEYBOARD_RDY		0x01		/* keyboard has a character */
#define KEYBOARD_IEN		0x02		/* enable keyboard interrupt */

struct ScanCodeEntry {
	uint8_t def;
	uint8_t ext;
};

static ScanCodeEntry scanCode2KeyCode[] = {
	/* 00 */	{0,				0},
	/* 01 */	{VK_F9,			0},
	/* 02 */	{0,				0},
	/* 03 */	{VK_F5,			0},
	/* 04 */	{VK_F3,			0},
	/* 05 */	{VK_F1,			0},
	/* 06 */	{VK_F2,			0},
	/* 07 */	{VK_F12,		0},
	/* 08 */	{0,				0},
	/* 09 */	{VK_F10,		0},
	/* 0A */	{VK_F8,			0},
	/* 0B */	{VK_F6,			0},
	/* 0C */	{VK_F4,			0},
	/* 0D */	{VK_TAB,		0},
	/* 0E */	{VK_ACCENT,		0},
	/* 0F */	{0,				0},
	/* 10 */	{0,				0},
	/* 11 */	{VK_LALT,		VK_RALT},
	/* 12 */	{VK_LSHIFT,		0},
	/* 13 */	{0,				0},
	/* 14 */	{VK_LCTRL,		VK_RCTRL},
	/* 15 */	{VK_Q,			0},
	/* 16 */	{VK_1,			0},
	/* 17 */	{0,				0},
	/* 18 */	{0,				0},
	/* 19 */	{0,				0},
	/* 1A */	{VK_Z,			0},
	/* 1B */	{VK_S,			0},
	/* 1C */	{VK_A,			0},
	/* 1D */	{VK_W,			0},
	/* 1E */	{VK_2,			0},
	/* 1F */	{0,				VK_LSUPER},
	/* 20 */	{0,				0},
	/* 21 */	{VK_C,			0},
	/* 22 */	{VK_X,			0},
	/* 23 */	{VK_D,			0},
	/* 24 */	{VK_E,			0},
	/* 25 */	{VK_4,			0},
	/* 26 */	{VK_3,			0},
	/* 27 */	{0,				VK_RSUPER},
	/* 28 */	{0,				0},
	/* 29 */	{VK_SPACE,		0},
	/* 2A */	{VK_V,			0},
	/* 2B */	{VK_F,			0},
	/* 2C */	{VK_T,			0},
	/* 2D */	{VK_R,			0},
	/* 2E */	{VK_5,			0},
	/* 2F */	{0,				VK_APPS},
	/* 30 */	{0,				0},
	/* 31 */	{VK_N,			0},
	/* 32 */	{VK_B,			0},
	/* 33 */	{VK_H,			0},
	/* 34 */	{VK_G,			0},
	/* 35 */	{VK_Y,			0},
	/* 36 */	{VK_6,			0},
	/* 37 */	{0,				0},
	/* 38 */	{0,				0},
	/* 39 */	{0,				0},
	/* 3A */	{VK_M,			0},
	/* 3B */	{VK_J,			0},
	/* 3C */	{VK_U,			0},
	/* 3D */	{VK_7,			0},
	/* 3E */	{VK_8,			0},
	/* 3F */	{0,				0},
	/* 40 */	{0,				0},
	/* 41 */	{VK_COMMA,		0},
	/* 42 */	{VK_K,			0},
	/* 43 */	{VK_I,			0},
	/* 44 */	{VK_O,			0},
	/* 45 */	{VK_0,			0},
	/* 46 */	{VK_9,			0},
	/* 47 */	{0,				0},
	/* 48 */	{0,				0},
	/* 49 */	{VK_DOT,		0},
	/* 4A */	{VK_SLASH,		VK_KPDIV},
	/* 4B */	{VK_L,			0},
	/* 4C */	{VK_SEM,		0},
	/* 4D */	{VK_P,			0},
	/* 4E */	{VK_MINUS,		0},
	/* 4F */	{0,				0},
	/* 50 */	{0,				0},
	/* 51 */	{0,				0},
	/* 52 */	{VK_APOS,		0},
	/* 53 */	{0,				0},
	/* 54 */	{VK_LBRACKET,	0},
	/* 55 */	{VK_EQ,			0},
	/* 56 */	{0,				0},
	/* 57 */	{0,				0},
	/* 58 */	{VK_CAPS,		0},
	/* 59 */	{VK_RSHIFT,		0},
	/* 5A */	{VK_ENTER,		VK_KPENTER},
	/* 5B */	{VK_RBRACKET,	0},
	/* 5C */	{0,				0},
	/* 5D */	{VK_BACKSLASH,	0},
	/* 5E */	{0,				0},
	/* 5F */	{0,				0},
	/* 60 */	{0,				0},
	/* 61 */	{VK_PIPE,		0},
	/* 62 */	{0,				0},
	/* 63 */	{0,				0},
	/* 64 */	{0,				0},
	/* 65 */	{0,				0},
	/* 66 */	{VK_BACKSP,		0},
	/* 67 */	{0,				0},
	/* 68 */	{0,				0},
	/* 69 */	{VK_KP1,		VK_END},
	/* 6A */	{0,				0},
	/* 6B */	{VK_KP4,		VK_LEFT},
	/* 6C */	{VK_KP7,		VK_HOME},
	/* 6D */	{0,				0},
	/* 6E */	{0,				0},
	/* 6F */	{0,				0},
	/* 70 */	{VK_KP0,		VK_INSERT},
	/* 71 */	{VK_KPDOT,		VK_DELETE},
	/* 72 */	{VK_KP2,		VK_DOWN},
	/* 73 */	{VK_KP5,		0},
	/* 74 */	{VK_KP6,		VK_RIGHT},
	/* 75 */	{VK_KP8,		VK_UP},
	/* 76 */	{VK_ESC,		0},
	/* 77 */	{VK_NUM,		0},
	/* 78 */	{VK_F11,		0},
	/* 79 */	{VK_KPADD,		0},
	/* 7A */	{VK_KP3,		VK_PGDOWN},
	/* 7B */	{VK_KPSUB,		0},
	/* 7C */	{VK_KPMUL,		0},
	/* 7D */	{VK_KP9,		VK_PGUP},
	/* 7E */	{VK_SCROLL,		0},
	/* 7F */	{0,				0},
	/* 80 */	{0,				0},
	/* 81 */	{0,				0},
	/* 82 */	{0,				0},
	/* 83 */	{VK_F7,			0},
};

uint8_t Keyboard::getKeyCode(uint *pflags) {
	static bool isExt = 0;
	static bool isBreak = false;
	uint64_t *kb = (uint64_t*)(KEYBOARD_BASE | DIR_MAPPED_SPACE);
	if(!(kb[KEYBOARD_CTRL] & KEYBOARD_RDY))
		return VK_NOKEY;

	uint8_t scanCode = kb[KEYBOARD_DATA];
	/* extended code-start? */
	if(scanCode == 0xE0) {
		isExt = true;
		return VK_NOKEY;
	}
	/* break code? */
	if(scanCode == 0xF0) {
		isBreak = true;
		return VK_NOKEY;
	}

	/* get keycode */
	ScanCodeEntry *e = scanCode2KeyCode + (scanCode % 0x84);
	uint8_t keycode = isExt ? e->ext : e->def;
	if(isBreak)
		*pflags |= KE_BREAK;
	else
		*pflags &= ~KE_BREAK;
	isExt = false;
	isBreak = false;
	return keycode;
}
