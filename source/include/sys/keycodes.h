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

#pragma once

enum {
	SHIFT_MASK			= 1,
	CTRL_MASK			= 2,
	ALT_MASK			= 4,
};

enum {
	VK_ACCENT			= 1,
	VK_0				= 2,
	VK_1				= 3,
	VK_2				= 4,
	VK_3				= 5,
	VK_4				= 6,
	VK_5				= 7,
	VK_6				= 8,
	VK_7				= 9,
	VK_8				= 10,
	VK_9				= 11,
	VK_MINUS			= 12,
	VK_EQ				= 13,
	VK_BACKSP			= 15,
	VK_TAB				= 16,
	VK_Q				= 17,
	VK_W				= 18,
	VK_E				= 19,
	VK_R				= 20,
	VK_T				= 21,
	VK_Y				= 22,
	VK_U				= 23,
	VK_I				= 24,
	VK_O				= 25,
	VK_P				= 26,
	VK_LBRACKET			= 27,
	VK_RBRACKET			= 28,
	VK_BACKSLASH		= 29,
	VK_CAPS				= 30,
	VK_A				= 31,
	VK_S				= 32,
	VK_D				= 33,
	VK_F				= 34,
	VK_G				= 35,
	VK_H				= 36,
	VK_J				= 37,
	VK_K				= 38,
	VK_L				= 39,
	VK_SEM				= 40,
	VK_APOS				= 41,
	/* non-US-1 ?? */
	VK_ENTER			= 43,
	VK_LSHIFT			= 44,
	VK_Z				= 46,
	VK_X				= 47,
	VK_C				= 48,
	VK_V				= 49,
	VK_B				= 50,
	VK_N				= 51,
	VK_M				= 52,
	VK_COMMA			= 53,
	VK_DOT				= 54,
	VK_SLASH			= 55,
	VK_RSHIFT			= 57,
	VK_LCTRL			= 58,
	VK_LSUPER			= 59,
	VK_LALT				= 60,
	VK_SPACE			= 61,
	VK_RALT				= 62,
	VK_APPS				= 63,	/* ?? */
	VK_RCTRL			= 64,
	VK_RSUPER			= 65,
	VK_INSERT			= 75,
	VK_DELETE			= 76,
	VK_HOME				= 80,
	VK_END				= 81,
	VK_PGUP				= 85,
	VK_PGDOWN			= 86,
	VK_LEFT				= 79,
	VK_UP				= 83,
	VK_DOWN				= 84,
	VK_RIGHT			= 89,
	VK_NUM				= 90,
	VK_KP7				= 91,
	VK_KP4				= 92,
	VK_KP1				= 93,
	VK_KPDIV			= 95,
	VK_KP8				= 96,
	VK_KP5				= 97,
	VK_KP2				= 98,
	VK_KP0				= 99,
	VK_KPMUL			= 100,
	VK_KP9				= 101,
	VK_KP6				= 102,
	VK_KP3				= 103,
	VK_KPDOT			= 104,
	VK_KPSUB			= 105,
	VK_KPADD			= 106,
	VK_KPENTER			= 108,
	VK_ESC				= 110,
	VK_F1				= 112,
	VK_F2				= 113,
	VK_F3				= 114,
	VK_F4				= 115,
	VK_F5				= 116,
	VK_F6				= 117,
	VK_F7				= 118,
	VK_F8				= 119,
	VK_F9				= 120,
	VK_F10				= 121,
	VK_F11				= 122,
	VK_F12				= 123,
	VK_PRINT			= 124,
	VK_SCROLL			= 125,
	VK_PAUSE			= 126,
	VK_PIPE				= 127,

	VK_NOKEY			= 128,
};
