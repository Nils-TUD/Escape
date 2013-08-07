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

#include <sys/common.h>
#include <sys/arch/i586/ports.h>
#include <sys/dbg/kb.h>
#include <esc/keycodes.h>

#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64
#define STATUS_OUTBUF_FULL			(1 << 0)

struct ScanCodeEntry {
	uint8_t def;
	uint8_t ext;
};

static ScanCodeEntry scanCode2KeyCode[] = {
	/* 00 */	{0,				0},
	/* 01 */	{VK_ESC,		0},
	/* 02 */	{VK_1,			0},
	/* 03 */	{VK_2,			0},
	/* 04 */	{VK_3,			0},
	/* 05 */	{VK_4,			0},
	/* 06 */	{VK_5,			0},
	/* 07 */	{VK_6,			0},
	/* 08 */	{VK_7,			0},
	/* 09 */	{VK_8,			0},
	/* 0A */	{VK_9,			0},
	/* 0B */	{VK_0,			0},
	/* 0C */	{VK_MINUS,		0},
	/* 0D */	{VK_EQ,			0},
	/* 0E */	{VK_BACKSP,		0},
	/* 0F */	{VK_TAB,		0},
	/* 10 */	{VK_Q,			0},
	/* 11 */	{VK_W,			0},
	/* 12 */	{VK_E,			0},
	/* 13 */	{VK_R,			0},
	/* 14 */	{VK_T,			0},
	/* 15 */	{VK_Y,			0},
	/* 16 */	{VK_U,			0},
	/* 17 */	{VK_I,			0},
	/* 18 */	{VK_O,			0},
	/* 19 */	{VK_P,			0},
	/* 1A */	{VK_LBRACKET,	0},
	/* 1B */	{VK_RBRACKET,	0},
	/* 1C */	{VK_ENTER,		VK_KPENTER},
	/* 1D */	{VK_LCTRL,		VK_RCTRL},
	/* 1E */	{VK_A,			0},
	/* 1F */	{VK_S,			0},
	/* 20 */	{VK_D,			0},
	/* 21 */	{VK_F,			0},
	/* 22 */	{VK_G,			0},
	/* 23 */	{VK_H,			0},
	/* 24 */	{VK_J,			0},
	/* 25 */	{VK_K,			0},
	/* 26 */	{VK_L,			0},
	/* 27 */	{VK_SEM,		0},
	/* 28 */	{VK_APOS,		0},
	/* 29 */	{VK_ACCENT,		0},
	/* 2A */	{VK_LSHIFT,		0},
	/* 2B */	{VK_BACKSLASH,	0},
	/* 2C */	{VK_Z,			0},
	/* 2D */	{VK_X,			0},
	/* 2E */	{VK_C,			0},
	/* 2F */	{VK_V,			0},
	/* 30 */	{VK_B,			0},
	/* 31 */	{VK_N,			0},
	/* 32 */	{VK_M,			0},
	/* 33 */	{VK_COMMA,		0},
	/* 34 */	{VK_DOT,		0},
	/* 35 */	{VK_SLASH,		VK_KPDIV},
	/* 36 */	{VK_RSHIFT,		0},
	/* 37 */	{VK_KPMUL,		0},
	/* 38 */	{VK_LALT,		VK_RALT},
	/* 39 */	{VK_SPACE,		0},
	/* 3A */	{VK_CAPS,		0},
	/* 3B */	{VK_F1,			0},
	/* 3C */	{VK_F2,			0},
	/* 3D */	{VK_F3,			0},
	/* 3E */	{VK_F4,			0},
	/* 3F */	{VK_F5,			0},
	/* 40 */	{VK_F6,			0},
	/* 41 */	{VK_F7,			0},
	/* 42 */	{VK_F8,			0},
	/* 43 */	{VK_F9,			0},
	/* 44 */	{VK_F10,		0},
	/* 45 */	{VK_NUM,		0},
	/* 46 */	{VK_SCROLL,		0},
	/* 47 */	{VK_KP7,		VK_HOME},
	/* 48 */	{VK_KP8,		VK_UP},
	/* 49 */	{VK_KP9,		VK_PGUP},
	/* 4A */	{VK_KPSUB,		0},
	/* 4B */	{VK_KP4,		VK_LEFT},
	/* 4C */	{VK_KP5,		0},
	/* 4D */	{VK_KP6,		VK_RIGHT},
	/* 4E */	{VK_KPADD,		0},
	/* 4F */	{VK_KP1,		VK_END},
	/* 50 */	{VK_KP2,		VK_DOWN},
	/* 51 */	{VK_KP3,		VK_PGDOWN},
	/* 52 */	{VK_KP0,		VK_INSERT},
	/* 53 */	{VK_KPDOT,		VK_DELETE},
	/* 54 */	{0,				0},
	/* 55 */	{0,				0},
	/* 56 */	{VK_PIPE,		0},
	/* 57 */	{VK_F11,		0},
	/* 58 */	{VK_F12,		0},
	/* 59 */	{0,				0},
	/* 5A */	{0,				0},
	/* 5B */	{0,				VK_LSUPER},
	/* 5C */	{0,				VK_RSUPER},
	/* 5D */	{0,				VK_APPS},
	/* 5E */	{0,				0},
	/* 5F */	{0,				0},
	/* 60 */	{0,				0},
	/* 61 */	{0,				0},
	/* 62 */	{0,				0},
	/* 63 */	{0,				0},
	/* 64 */	{0,				0},
	/* 65 */	{0,				0},
	/* 66 */	{0,				0},
	/* 67 */	{0,				0},
	/* 68 */	{0,				0},
	/* 69 */	{0,				0},
	/* 6A */	{0,				0},
	/* 6B */	{0,				0},
	/* 6C */	{0,				0},
	/* 6D */	{0,				0},
	/* 6E */	{0,				0},
	/* 6F */	{0,				0},
	/* 70 */	{0,				0},
	/* 71 */	{0,				0},
	/* 72 */	{0,				0},
	/* 73 */	{0,				0},
	/* 74 */	{0,				0},
	/* 75 */	{0,				0},
	/* 76 */	{0,				0},
	/* 77 */	{0,				0},
	/* 78 */	{0,				0},
	/* 79 */	{0,				0},
	/* 7A */	{0,				0},
	/* 7B */	{0,				0},
	/* 7C */	{0,				0},
	/* 7D */	{0,				0},
	/* 7E */	{0,				0},
	/* 7F */	{0,				0},
};

uint8_t Keyboard::getKeyCode(uint *pflags) {
	static uint8_t set = 0;
	uint8_t status = Ports::in<uint8_t>(IOPORT_KB_CTRL);
	if(!(status & STATUS_OUTBUF_FULL))
		return VK_NOKEY;

	uint8_t scanCode = Ports::in<uint8_t>(IOPORT_KB_DATA);
	/* extended code-start? */
	if(scanCode == 0xE0) {
		set = 1;
		return VK_NOKEY;
	}

	/* break? */
	*pflags &= ~KE_BREAK;
	if(scanCode & 0x80) {
		*pflags |= KE_BREAK;
		scanCode &= ~0x80;
	}

	/* get keycode */
	ScanCodeEntry *e = scanCode2KeyCode + (scanCode & 0x7F);
	uint8_t keycode = set ? e->ext : e->def;
	set = 0;
	return keycode;
}
