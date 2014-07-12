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

#include <common.h>
#include <dbg/kb.h>
#include <sys/keycodes.h>

uint Keyboard::flags = 0;
Keyboard::KeymapEntry Keyboard::keymap[] = {
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
	/* VK_KPDOT */				{',',',','\0'},
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

bool Keyboard::get(Event *ev,uint8_t events,bool wait) {
	while(true) {
		uint8_t keycode = getKeyCode(&flags);
		if(keycode == VK_NOKEY) {
			if(!wait)
				break;
			while((keycode = getKeyCode(&flags)) == VK_NOKEY)
				;
		}

		/* key pressed/released */
		if(translate(ev,keycode)) {
			if(((flags & KE_BREAK) && (events & KEV_RELEASE)) ||
				(!(flags & KE_BREAK) && (events & KEV_PRESS)))
				return true;
		}
	}
	return false;
}

bool Keyboard::translate(Event *ev,uint8_t keycode) {
	if(ev)
		ev->keycode = keycode;

	/* handle shift,ctrl,alt */
	KeymapEntry *km = keymap + keycode;
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			flags = toggleFlag(flags & KE_BREAK,flags,KE_SHIFT);
			break;
		case VK_LALT:
		case VK_RALT:
			flags = toggleFlag(flags & KE_BREAK,flags,KE_ALT);
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			flags = toggleFlag(flags & KE_BREAK,flags,KE_CTRL);
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
	return true;
}

uint8_t Keyboard::toggleFlag(bool isbreak,uint8_t val,uint8_t flag) {
	if(isbreak)
		return val & ~flag;
	return val | flag;
}
