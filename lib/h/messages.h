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

#define CURSOR_DEFAULT				0
#define CURSOR_RESIZE_L				1
#define CURSOR_RESIZE_BR			2
#define CURSOR_RESIZE_VERT			3
#define CURSOR_RESIZE_BL			4
#define CURSOR_RESIZE_R				5
#define CURSOR_RESIZE_WIDTH			10

/* == Messages handled by the kernel == */
/* fs */
#define MSG_FS_OPEN_RESP			0
#define MSG_FS_READ_RESP			1
#define MSG_FS_WRITE_RESP			2
#define MSG_FS_CLOSE_RESP			3
#define MSG_FS_STAT_RESP			4
#define MSG_FS_LINK_RESP			5
#define MSG_FS_UNLINK_RESP			6
#define MSG_FS_MKDIR_RESP			7
#define MSG_FS_RMDIR_RESP			8
#define MSG_FS_MOUNT_RESP			9
#define MSG_FS_UNMOUNT_RESP			10
#define MSG_FS_ISTAT_RESP			11
/* driver */
#define MSG_DRV_OPEN_RESP			12
#define MSG_DRV_READ_RESP			13
#define MSG_DRV_WRITE_RESP			14
#define MSG_DRV_IOCTL_RESP			15
#define MSG_DRV_CLOSE_RESP			16
/* requests to fs */
#define MSG_FS_OPEN					200
#define MSG_FS_READ					201
#define MSG_FS_WRITE				202
#define MSG_FS_CLOSE				203
#define MSG_FS_STAT					204
#define MSG_FS_SYNC					205
#define MSG_FS_LINK					206
#define MSG_FS_UNLINK				207
#define MSG_FS_MKDIR				208
#define MSG_FS_RMDIR				209
#define MSG_FS_MOUNT				210
#define MSG_FS_UNMOUNT				211
#define MSG_FS_ISTAT				212
/* requests to driver */
#define MSG_DRV_OPEN				300
#define MSG_DRV_READ				301
#define MSG_DRV_WRITE				302
#define MSG_DRV_IOCTL				303
#define MSG_DRV_CLOSE				304

/* == Other messages == */
#define MSG_KEYBOARD_READ			400
#define MSG_KEYBOARD_DATA			401

#define MSG_VIDEO_SET				500
#define MSG_VIDEO_SETSCREEN			501
#define MSG_VIDEO_SETCURSOR			502

#define MSG_SPEAKER_BEEP			600

#define MSG_ATA_READ_REQ			700
#define MSG_ATA_WRITE_REQ			701
#define MSG_ATA_READ_RESP			702

#define MSG_ENV_GET					800
#define MSG_ENV_SET					801
#define MSG_ENV_GET_RESP			802
#define MSG_ENV_GETI				803

#define MSG_VESA_UPDATE				900
#define MSG_VESA_CURSOR				901
#define MSG_VESA_GETMODE_REQ		902
#define MSG_VESA_GETMODE_RESP		903

#define MSG_MOUSE					1000

#define MSG_WIN_CREATE_REQ			1100
#define MSG_WIN_CREATE_RESP			1101
#define MSG_WIN_MOUSE				1102
#define MSG_WIN_MOVE_REQ			1103
#define MSG_WIN_UPDATE				1104
#define MSG_WIN_KEYBOARD			1105
#define MSG_WIN_SET_ACTIVE			1106
#define MSG_WIN_DESTROY_REQ			1107
#define MSG_WIN_UPDATE_REQ			1108
#define MSG_WIN_RESIZE_REQ			1109

#define MSG_RECEIVE					1200
#define MSG_SEND					1201

#define MSG_POWER_REBOOT			1300
#define MSG_POWER_SHUTDOWN			1301

/* the data read from the keyboard */
typedef struct {
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
} sKbData;

typedef struct {
	/* wether the key was released */
	u8 isBreak;
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* modifiers (STATE_CTRL, STATE_SHIFT, STATE_ALT) */
	u8 modifier;
	/* the character, translated by the current keymap */
	char character;
} sKmData;

/* the data read from the mouse */
typedef struct {
	s8 x;
	s8 y;
	u8 buttons;
} sMouseData;

typedef struct {
	u16 width;					/* x-resolution */
	u16 height;					/* y-resolution */
	u8 bitsPerPixel;			/* Bits per pixel                  */
	u8 redMaskSize;				/* Size of direct color red mask   */
	u8 redFieldPosition;		/* Bit posn of lsb of red mask     */
	u8 greenMaskSize;			/* Size of direct color green mask */
	u8 greenFieldPosition;		/* Bit posn of lsb of green mask   */
	u8 blueMaskSize;			/* Size of direct color blue mask  */
	u8 blueFieldPosition;		/* Bit posn of lsb of blue mask    */
} sVESAInfo;

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
