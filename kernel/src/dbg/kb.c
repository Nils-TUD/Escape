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

#include <sys/common.h>
#include <sys/dbg/kb.h>
#include <sys/util.h>
#include <esc/keycodes.h>

#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64
#define STATUS_OUTBUF_FULL			(1 << 0)

typedef struct {
	u8 def;
	u8 ext;
} sScanCodeEntry;

typedef struct {
	char def;
	char shift;
	char alt;
} sKeymapEntry;

static bool kb_translate(sKeyEvent *ev,u8 scanCode);
static u8 kb_toggleFlag(u8 isbreak,u8 val,u8 flag);

static sScanCodeEntry scanCode2KeyCode[] = {
	/* 00 */	{0,				0},
	/* 01 */	{0,				0},
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

static sKeymapEntry keymap[] = {
	/* --- */					{'\0','\0','\0'},
	/* VK_ACCENT ( ° ) */		{'^','\xF8','\0'},
	/* VK_0 */					{'0','=','}'},
	/* VK_1 */					{'1','!','\0'},
	/* VK_2 ( ² ) */			{'2','"','\xFD'},
	/* VK_3 ( § and ³ ) */		{'3','\x15','\0'},
	/* VK_4 */					{'4','$','\0'},
	/* VK_5 */					{'5','%','\0'},
	/* VK_6 */					{'6','&','\0'},
	/* VK_7 */					{'7','/','{'},
	/* VK_8 */					{'8','(','['},
	/* VK_9 */					{'9',')',']'},
	/* VK_MINUS ( ß ) */		{'\xE1','?','\\'},
	/* VK_EQ */					{'\'','`','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_BACKSP */				{'\b','\b','\0'},
	/* VK_TAB */				{'\t','\t','\0'},
	/* VK_Q */					{'q','Q','@'},
	/* VK_W */					{'w','W','\0'},
	/* VK_E ( € ) */			{'e','E','\0'},
	/* VK_R */					{'r','R','\0'},
	/* VK_T */					{'t','T','\0'},
	/* VK_Y */					{'z','Z','\0'},
	/* VK_U */					{'u','U','\0'},
	/* VK_I */					{'i','I','\0'},
	/* VK_O */					{'o','O','\0'},
	/* VK_P */					{'p','P','\0'},
	/* VK_LBRACKET (ü and Ü) */ {'\x81','\x9A','\0'},
	/* VK_RBRACKET */			{'+','*','~'},
	/* VK_BACKSLASH */			{'#','\'','\0'},
	/* VK_CAPS */				{'\0','\0','\0'},
	/* VK_A */					{'a','A','\0'},
	/* VK_S */					{'s','S','\0'},
	/* VK_D */					{'d','D','\0'},
	/* VK_F */					{'f','F','\0'},
	/* VK_G */					{'g','G','\0'},
	/* VK_H */					{'h','H','\0'},
	/* VK_J */					{'j','J','\0'},
	/* VK_K */					{'k','K','\0'},
	/* VK_L */					{'l','L','\0'},
	/* VK_SEM ( ö and Ö ) */	{'\x94','\x99','\0'},
	/* VK_APOS ( ä and Ä ) */	{'\x84','\x8E','\0'},
	/* non-US-1 */				{'\0','\0','\0'},
	/* VK_ENTER */				{'\n','\n','\0'},
	/* VK_LSHIFT */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_Z */					{'y','Y','\0'},
	/* VK_X */					{'x','X','\0'},
	/* VK_C */					{'c','C','\0'},
	/* VK_V */					{'v','V','\0'},
	/* VK_B */					{'b','B','\0'},
	/* VK_N */					{'n','N','\0'},
	/* VK_M ( µ ) */			{'m','M','\xE6'},
	/* VK_COMMA */				{',',';','\0'},
	/* VK_DOT */				{'.',':','\0'},
	/* VK_SLASH */				{'-','_','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_RSHIFT */				{'\0','\0','\0'},
	/* VK_LCTRL */				{'\0','\0','\0'},
	/* VK_LSUPER */				{'\0','\0','\0'},
	/* VK_LALT */				{'\0','\0','\0'},
	/* VK_SPACE */				{' ',' ','\0'},
	/* VK_RALT */				{'\0','\0','\0'},
	/* VK_APPS */				{'\0','\0','\0'},
	/* VK_RCTRL */				{'\0','\0','\0'},
	/* VK_RSUPER */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_INSERT */				{'\0','\0','\0'},
	/* VK_DELETE */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_LEFT */				{'\0','\0','\0'},
	/* VK_HOME */				{'\0','\0','\0'},
	/* VK_END */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_UP */					{'\0','\0','\0'},
	/* VK_DOWN */				{'\0','\0','\0'},
	/* VK_PGUP */				{'\0','\0','\0'},
	/* VK_PGDOWN */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_RIGHT */				{'\0','\0','\0'},
	/* VK_NUM */				{'\0','\0','\0'},
	/* VK_KP7 */				{'7','7','\0'},
	/* VK_KP4 */				{'4','4','\0'},
	/* VK_KP1 */				{'1','1','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_KPDIV */				{'/','/','\0'},
	/* VK_KP8 */				{'8','8','\0'},
	/* VK_KP5 */				{'5','5','\0'},
	/* VK_KP2 */				{'2','2','\0'},
	/* VK_KP0 */				{'0','0','\0'},
	/* VK_KPMUL */				{'*','*','\0'},
	/* VK_KP9 */				{'9','9','\0'},
	/* VK_KP6 */				{'6','6','\0'},
	/* VK_KP3 */				{'3','3','\0'},
	/* VK_KPDOT */				{'.','.','\0'},
	/* VK_KPSUB */				{'-','-','\0'},
	/* VK_KPADD */				{'+','+','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_KPENTER */			{'\n','\n','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_ESC */				{'\0','\0','\0'},
	/* --- */					{'\0','\0','\0'},
	/* VK_F1 */					{'\0','\0','\0'},
	/* VK_F2 */					{'\0','\0','\0'},
	/* VK_F3 */					{'\0','\0','\0'},
	/* VK_F4 */					{'\0','\0','\0'},
	/* VK_F5 */					{'\0','\0','\0'},
	/* VK_F6 */					{'\0','\0','\0'},
	/* VK_F7 */					{'\0','\0','\0'},
	/* VK_F8 */					{'\0','\0','\0'},
	/* VK_F9 */					{'\0','\0','\0'},
	/* VK_F10 */				{'\0','\0','\0'},
	/* VK_F11 */				{'\0','\0','\0'},
	/* VK_F12 */				{'\0','\0','\0'},
	/* VK_PRINT */				{'\0','\0','\0'},
	/* VK_SCROLL */				{'\0','\0','\0'},
	/* VK_PAUSE */				{'\0','\0','\0'},
	/* VK_PIPE */				{'<','>','|'},
};

static u8 set = 0;
static u8 flags = 0;

bool kb_get(sKeyEvent *ev,u8 events,bool wait) {
	while(true) {
		u8 status = util_inByte(IOPORT_KB_CTRL);
		if(!(status & STATUS_OUTBUF_FULL)) {
			if(!wait)
				break;
			do
				status = util_inByte(IOPORT_KB_CTRL);
			while(!(status & STATUS_OUTBUF_FULL));
		}
		if(kb_translate(ev,util_inByte(IOPORT_KB_DATA))) {
			if(((flags & KE_BREAK) && (events & KEV_RELEASE)) ||
				(!(flags & KE_BREAK) && (events & KEV_PRESS)))
				return true;
		}
	}
	return false;
}

static bool kb_translate(sKeyEvent *ev,u8 scanCode) {
	u8 keycode;
	sScanCodeEntry *e;
	sKeymapEntry *km;
	/* extended code-start? */
	if(scanCode == 0xE0) {
		set = 1;
		return false;
	}

	/* break? */
	flags &= ~KE_BREAK;
	if(scanCode & 0x80) {
		flags |= KE_BREAK;
		scanCode &= ~0x80;
	}

	/* get keycode */
	e = scanCode2KeyCode + (scanCode & 0x7F);
	keycode = set ? e->ext : e->def;
	if(ev)
		ev->keycode = keycode;

	/* handle shift,ctrl,alt */
	km = keymap + keycode;
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			flags = kb_toggleFlag(flags & KE_BREAK,flags,KE_SHIFT);
			break;
		case VK_LALT:
		case VK_RALT:
			flags = kb_toggleFlag(flags & KE_BREAK,flags,KE_ALT);
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			flags = kb_toggleFlag(flags & KE_BREAK,flags,KE_CTRL);
			break;
	}
	if(ev)
		ev->flags = flags;

	/* fetch char from keymap */
	if(ev) {
		if(flags & KE_SHIFT)
			ev->character = km->shift;
		else if(flags & KE_ALT)
			ev->character = km->alt;
		else
			ev->character = km->def;
	}

	set = 0;
	return true;
}

static u8 kb_toggleFlag(u8 isbreak,u8 val,u8 flag) {
	if(isbreak)
		return val & ~flag;
	return val | flag;
}
