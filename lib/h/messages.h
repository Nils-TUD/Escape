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

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <types.h>
#include <stddef.h>
#include <fsinterface.h>

/* general */
#define MAX_MSG_ARGS				10
#define MAX_MSG_SIZE				(1024 + 4 * sizeof(u32))
#define MAX_MSGSTR_LEN				64

/* == Messages handled by the kernel == */
/* fs */
#define MSG_FS_OPEN_RESP			0
#define MSG_FS_READ_RESP			1
#define MSG_FS_WRITE_RESP			2
#define MSG_FS_CLOSE_RESP			3
#define MSG_FS_STAT_RESP			4
/* driver */
#define MSG_DRV_OPEN_RESP			5
#define MSG_DRV_READ_RESP			6
#define MSG_DRV_WRITE_RESP			7
#define MSG_DRV_IOCTL_RESP			8
#define MSG_DRV_CLOSE_RESP			9
/* requests to fs */
#define MSG_FS_OPEN					20
#define MSG_FS_READ					21
#define MSG_FS_WRITE				22
#define MSG_FS_CLOSE				23
#define MSG_FS_STAT					24
#define MSG_FS_SYNC					25
/* requests to driver */
#define MSG_DRV_OPEN				40
#define MSG_DRV_READ				41
#define MSG_DRV_WRITE				42
#define MSG_DRV_IOCTL				43
#define MSG_DRV_CLOSE				44

/* == Other messages == */
#define MSG_KEYBOARD_READ			60
#define MSG_KEYBOARD_DATA			61

#define MSG_VIDEO_SET				80
#define MSG_VIDEO_SETSCREEN			81
#define MSG_VIDEO_SETCURSOR			82

#define MSG_SPEAKER_BEEP			100

#define MSG_ATA_READ_REQ			120
#define MSG_ATA_WRITE_REQ			121
#define MSG_ATA_READ_RESP			122

#define MSG_ENV_GET					140
#define MSG_ENV_SET					141
#define MSG_ENV_GET_RESP			142
#define MSG_ENV_GETI				143

#define MSG_VESA_UPDATE				160
#define MSG_VESA_CURSOR				161
#define MSG_VESA_GETMODE_REQ		162
#define MSG_VESA_GETMODE_RESP		163

#define MSG_MOUSE					180

#define MSG_WIN_CREATE_REQ			200
#define MSG_WIN_CREATE_RESP			201
#define MSG_WIN_MOUSE				202
#define MSG_WIN_MOVE_REQ			203
#define MSG_WIN_UPDATE				204
#define MSG_WIN_KEYBOARD			205
#define MSG_WIN_SET_ACTIVE			206
#define MSG_WIN_DESTROY_REQ			207
#define MSG_WIN_UPDATE_REQ			208

#define MSG_RECEIVE					220
#define MSG_SEND					221

/* the data read from the keyboard */
typedef struct {
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
} sKbData;

/* the data read from the mouse */
typedef struct {
	s8 x;
	s8 y;
	u8 buttons;
} sMouseData;

/* the message we're using for communication */
typedef union {
	/* for messages with integer arguments only */
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		u32 arg5;
		u32 arg6;
	} args;
	/* for messages with a few integer arguments and one or two strings */
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		char s1[MAX_MSGSTR_LEN];
		char s2[MAX_MSGSTR_LEN];
	} str;
	/* for messages with a few integer arguments and a data-part */
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		char d[MAX_MSG_SIZE - sizeof(u32) * 4];
	} data;
} sMsg;

#endif /* MESSAGES_H_ */
