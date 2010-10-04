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
#include <esc/fsinterface.h>

/* general */
#define MAX_MSG_ARGS				10
#define MAX_MSG_SIZE				128
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
#define MSG_DRV_CLOSE_RESP			15
/* default response */
#define MSG_DEF_RESPONSE			100
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
#define MSG_DRV_CLOSE				303

/* == Other messages == */
#define MSG_SPEAKER_BEEP			600

#define MSG_ENV_GET					800
#define MSG_ENV_SET					801
#define MSG_ENV_GET_RESP			802
#define MSG_ENV_GETI				803

#define MSG_VESA_UPDATE				900
#define MSG_VESA_CURSOR				901
#define MSG_VESA_GETMODE_REQ		902
#define MSG_VESA_GETMODE_RESP		903
#define MSG_VESA_SETMODE			904
#define MSG_VESA_ENABLE				905
#define MSG_VESA_DISABLE			906

#define MSG_MOUSE					1000

#define MSG_WIN_CREATE_REQ			1100
#define MSG_WIN_CREATE_RESP			1101
#define MSG_WIN_MOUSE				1102
#define MSG_WIN_MOVE				1103
#define MSG_WIN_UPDATE_REQ			1104
#define MSG_WIN_KEYBOARD			1105
#define MSG_WIN_SET_ACTIVE			1106
#define MSG_WIN_DESTROY				1107
#define MSG_WIN_UPDATE				1108
#define MSG_WIN_RESIZE				1109
#define MSG_WIN_ENABLE				1110
#define MSG_WIN_DISABLE				1111

#define MSG_POWER_REBOOT			1300
#define MSG_POWER_SHUTDOWN			1301

#define MSG_VID_SETCURSOR			1400	/* expects sVTPos */
#define MSG_VID_GETSIZE				1401	/* writes into sVTSize */
#define MSG_VID_SETMODE				1402

#define MSG_VT_EN_ECHO				1500
#define MSG_VT_DIS_ECHO				1501
#define MSG_VT_EN_RDLINE			1502
#define MSG_VT_DIS_RDLINE			1503
#define MSG_VT_EN_RDKB				1504
#define MSG_VT_DIS_RDKB				1505
#define MSG_VT_EN_NAVI				1506
#define MSG_VT_DIS_NAVI				1507
#define MSG_VT_BACKUP				1508
#define MSG_VT_RESTORE				1509
#define MSG_VT_SHELLPID				1510
#define MSG_VT_GETSIZE				1511	/* writes into sVTSize */
#define MSG_VT_ENABLE				1512	/* enables vterm */
#define MSG_VT_DISABLE				1513	/* disables vterm */
#define MSG_VT_SELECT				1514	/* selects the vterm */

#define MSG_KM_SET					1600	/* sets a keymap, expects the keymap-path as argument */
#define MSG_KM_EVENT				1601	/* the message-id for sending events to the listeners */

#define MSG_KE_ADDLISTENER			1700
#define MSG_KE_REMLISTENER			1701

#define MSG_PCI_GET_BY_CLASS		1800
#define MSG_PCI_GET_BY_ID			1801
#define MSG_PCI_DEVICE_RESP			1802

/* the possible km-events to listen to; KE_EV_PRESSED, KE_EV_RELEASED and KE_EV_KEYCODE,
 * KE_EV_CHARACTER are mutually exclusive, each */
#define KE_EV_PRESSED				1
#define KE_EV_RELEASED				2
#define KE_EV_KEYCODE				4
#define KE_EV_CHARACTER				8

/* the data read from the keyboard */
typedef struct {
	/* the keycode (see keycodes.h) */
	uchar keycode;
	/* whether the key was released */
	uchar isBreak;
} sKbData;

typedef struct {
	/* whether the key was released */
	uchar isBreak;
	/* the keycode (see keycodes.h) */
	uchar keycode;
	/* modifiers (STATE_CTRL, STATE_SHIFT, STATE_ALT) */
	uchar modifier;
	/* the character, translated by the current keymap */
	char character;
} sKmData;

/* the data read from the mouse */
typedef struct {
	short x;
	short y;
	uchar buttons;
} sMouseData;

typedef struct {
	ushort width;					/* x-resolution */
	ushort height;					/* y-resolution */
	uchar bitsPerPixel;				/* Bits per pixel                  */
	uchar redMaskSize;				/* Size of direct color red mask   */
	uchar redFieldPosition;			/* Bit posn of lsb of red mask     */
	uchar greenMaskSize;			/* Size of direct color green mask */
	uchar greenFieldPosition;		/* Bit posn of lsb of green mask   */
	uchar blueMaskSize;				/* Size of direct color blue mask  */
	uchar blueFieldPosition;		/* Bit posn of lsb of blue mask    */
} sVESAInfo;

typedef struct {
	uint col;
	uint row;
} sVTPos;

typedef struct {
	uint width;
	uint height;
} sVTSize;

typedef struct {
	enum {BAR_MEM,BAR_IO} type;
	uintptr_t addr;
	size_t size;
} sPCIBar;

typedef struct {
	uchar bus;
	uchar dev;
	uchar func;
	uchar type;
	ushort deviceId;
	ushort vendorId;
	uchar baseClass;
	uchar subClass;
	uchar progInterface;
	uchar revId;
	uchar irq;
	sPCIBar bars[6];
} sPCIDevice;

/* the message we're using for communication */
typedef union {
	/* for messages with integer arguments only */
	struct {
		uint arg1;
		uint arg2;
		uint arg3;
		uint arg4;
		uint arg5;
		uint arg6;
	} args;
	/* for messages with a few integer arguments and one or two strings */
	struct {
		uint arg1;
		uint arg2;
		uint arg3;
		uint arg4;
		char s1[MAX_MSGSTR_LEN];
		char s2[MAX_MSGSTR_LEN];
	} str;
	/* for messages with a few integer arguments and a data-part */
	struct {
		uint arg1;
		uint arg2;
		uint arg3;
		uint arg4;
		char d[MAX_MSG_SIZE];
	} data;
} sMsg;

#endif /* MESSAGES_H_ */
