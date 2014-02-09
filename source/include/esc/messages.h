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

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <stddef.h>
#include <time.h>

/* general */
#define MAX_MSG_ARGS				10
#define MAX_MSG_SIZE				128
#define MAX_MSGSTR_LEN				63

/* == messages == */
/* default response */
#define MSG_DEF_RESPONSE			100000

/* requests to device */
#define MSG_DEV_OPEN				50
#define MSG_DEV_READ				51
#define MSG_DEV_WRITE				52
#define MSG_DEV_CLOSE				53
#define MSG_DEV_SHFILE				54

/* respones of devices */
#define MSG_DEV_OPEN_RESP			100001
#define MSG_DEV_READ_RESP			100002
#define MSG_DEV_WRITE_RESP			100003
#define MSG_DEV_CLOSE_RESP			100004
#define MSG_DEV_SHFILE_RESP			100005

/* requests to fs */
#define MSG_FS_OPEN					100
#define MSG_FS_READ					101
#define MSG_FS_WRITE				102
#define MSG_FS_CLOSE				103
#define MSG_FS_STAT					104
#define MSG_FS_SYNCFS				105
#define MSG_FS_LINK					106
#define MSG_FS_UNLINK				107
#define MSG_FS_MKDIR				108
#define MSG_FS_RMDIR				109
#define MSG_FS_ISTAT				110
#define MSG_FS_CHMOD				111
#define MSG_FS_CHOWN				112

/* responses of fs */
#define MSG_FS_OPEN_RESP			100006
#define MSG_FS_READ_RESP			100007
#define MSG_FS_WRITE_RESP			100008
#define MSG_FS_STAT_RESP			100009
#define MSG_FS_SYNCFS_RESP			100010
#define MSG_FS_LINK_RESP			100011
#define MSG_FS_UNLINK_RESP			100012
#define MSG_FS_MKDIR_RESP			100013
#define MSG_FS_RMDIR_RESP			100014
#define MSG_FS_ISTAT_RESP			100015
#define MSG_FS_CHMOD_RESP			100016
#define MSG_FS_CHOWN_RESP			100017

/* == Other messages == */
#define MSG_SPEAKER_BEEP			200	/* performs a beep */

#define MSG_WIN_CREATE				300	/* creates a window */
#define MSG_WIN_CREATE_RESP			301	/* the create-response */
#define MSG_WIN_MOVE				302	/* moves a window */
#define MSG_WIN_UPDATE				303	/* requests an update of a window */
#define MSG_WIN_SET_ACTIVE_EV		304	/* sets the active window */
#define MSG_WIN_DESTROY				305	/* destroys a window */
#define MSG_WIN_RESIZE				306	/* resizes a window */
#define MSG_WIN_ENABLE				307	/* enables the window-manager */
#define MSG_WIN_DISABLE				308	/* disables the window-manager */
#define MSG_WIN_MOUSE_EV			309	/* a mouse-event sent by the window-manager */
#define MSG_WIN_KEYBOARD_EV			310	/* a keyboard-event sent by the window-manager */
#define MSG_WIN_CREATE_EV			311	/* a window has been created */
#define MSG_WIN_DESTROY_EV			312	/* a window has been destroyed */
#define MSG_WIN_ADDLISTENER			313	/* announces a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_REMLISTENER			314	/* removes a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_SET_ACTIVE			315	/* requests that a window is set to the active one */
#define MSG_WIN_ACTIVE_EV			316	/* a window has been set to active */
#define MSG_WIN_RESIZE_RESP			317	/* a resize has been finished */
#define MSG_WIN_UPDATE_RESP			318 /* an update has been finished */
#define MSG_WIN_ATTACH				319 /* connect an event-channel to the request-channel */
#define MSG_WIN_RESET_EV			320 /* asks the app to reset everything */
#define MSG_WIN_SETMODE				321 /* sets the screen mode */

#define MSG_SCR_SETCURSOR			500	/* sets the cursor */
#define MSG_SCR_GETMODE				501 /* gets information about the current video-mode */
#define MSG_SCR_SETMODE				502	/* sets the video-mode */
#define MSG_SCR_GETMODES            503 /* gets all video-modes */
#define MSG_SCR_UPDATE				504 /* updates a part of the screen */

#define MSG_VT_EN_ECHO				600	/* enables that the vterm echo's typed characters */
#define MSG_VT_DIS_ECHO				601	/* disables echo */
#define MSG_VT_EN_RDLINE			602	/* enables the readline-feature */
#define MSG_VT_DIS_RDLINE			603	/* disables the readline-feature */
#define MSG_VT_EN_NAVI				604	/* enables navigation (page-up, arrow-up, ... ) */
#define MSG_VT_DIS_NAVI				605	/* disables navigation */
#define MSG_VT_BACKUP				606	/* backups the screen */
#define MSG_VT_RESTORE				607	/* restores the screen */
#define MSG_VT_SHELLPID				608	/* gives the vterm the shell-pid */
#define MSG_VT_ISVTERM				609 /* dummy message on which only vterm answers with no error */
#define MSG_VT_SETMODE				610 /* requests vterm to set the video-mode */

#define MSG_KB_EVENT				700 /* events that the keyboard-driver sends */

#define MSG_MS_EVENT				800 /* events that the mouse-driver sends */

#define MSG_UIM_GETKEYMAP			900 /* gets the current keymap path */
#define MSG_UIM_SETKEYMAP			901	/* sets a keymap, expects the keymap-path as argument */
#define MSG_UIM_EVENT				902	/* the message-id for sending events to the listeners */
#define MSG_UIM_ATTACH				903 /* is used to attach to the ctrl-session */
#define MSG_UIM_GETID				904 /* get the id to use for attach */

#define MSG_PCI_GET_BY_CLASS		1000	/* searches for a pci device with given class */
#define MSG_PCI_GET_BY_ID			1001	/* searches for a pci device with given id */
#define MSG_PCI_GET_LIST			1002 /* get device-count or a device by index */

#define MSG_INIT_REBOOT				1100 /* requests a reboot */
#define MSG_INIT_SHUTDOWN			1101 /* requests a shutdown */
#define MSG_INIT_IAMALIVE			1102 /* tells init that the shutdown-request has been received
											and that you promise to terminate as soon as possible */

#define MSG_DISK_GETSIZE			1200 /* get the size of a device */


#define IS_DEVICE_MSG(id)			((id) == MSG_DEV_OPEN || \
									 (id) == MSG_DEV_READ || \
									 (id) == MSG_DEV_WRITE || \
									 (id) == MSG_DEV_CLOSE || \
									 (id) == MSG_DEV_SHFILE)

/* cursors */
#define CURSOR_DEFAULT				0
#define CURSOR_RESIZE_L				1
#define CURSOR_RESIZE_BR			2
#define CURSOR_RESIZE_VERT			3
#define CURSOR_RESIZE_BL			4
#define CURSOR_RESIZE_R				5
#define CURSOR_RESIZE_WIDTH			6

/* the modes */
#define VID_MODE_TEXT				0
#define VID_MODE_GRAPHICAL			1
/* the mode types */
#define VID_MODE_TYPE_TUI			1
#define VID_MODE_TYPE_GUI			2

/* the data send from the keyboard */
typedef struct {
	/* the keycode (see keycodes.h) */
	uchar keycode;
	/* whether the key was released */
	uchar isBreak;
} sKbData;

/* the data send from the mouse */
typedef struct {
	gpos_t x;
	gpos_t y;
	gpos_t z;
	uchar buttons;
} sMouseData;

/* the data send from the kmmanager */
typedef struct {
	enum {
		KM_EV_KEYBOARD,
		KM_EV_MOUSE
	} type;
	union {
		struct {
			/* the keycode (see keycodes.h) */
			uchar keycode;
			/* modifiers (STATE_CTRL, STATE_SHIFT, STATE_ALT, STATE_BREAK) */
			uchar modifier;
			/* the character, translated by the current keymap */
			char character;
		} keyb;
		sMouseData mouse;
	} d;
} sUIMData;

typedef struct {
	int id;
	uint cols;
	uint rows;
	uint width;
	uint height;
	uchar bitsPerPixel;
	uchar redMaskSize;				/* Size of direct color red mask   */
	uchar redFieldPosition;			/* Bit posn of lsb of red mask     */
	uchar greenMaskSize;			/* Size of direct color green mask */
	uchar greenFieldPosition;		/* Bit posn of lsb of green mask   */
	uchar blueMaskSize;				/* Size of direct color blue mask  */
	uchar blueFieldPosition;		/* Bit posn of lsb of blue mask    */
	uintptr_t physaddr;
	size_t tuiHeaderSize;
	size_t guiHeaderSize;
	int mode;
	int type;
} sScreenMode;

typedef struct {
	struct tm time;
	uint microsecs;
} sRTCInfo;

typedef struct {
	enum {BAR_MEM,BAR_IO} type;
	enum {
		BAR_MEM_32			= 0x1,
		BAR_MEM_64			= 0x2,
		BAR_MEM_PREFETCH	= 0x4
	};
	uintptr_t addr;
	size_t size;
	uint flags;
} sPCIBar;

typedef enum {
	PCI_TYPE_GENERIC			= 0,
	PCI_TYPE_PCI_PCI_BRIDGE		= 1,
	PCI_TYPE_CARD_BUS_BRIDGE	= 2,
} ePCIDevType;

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

/* for messages with integer arguments only */
typedef struct {
	ulong arg1;
	ulong arg2;
	ulong arg3;
	ulong arg4;
	ulong arg5;
	ulong arg6;
	ulong arg7;
	ulong arg8;
} sArgsMsg;

/* for messages with a few integer arguments and one or two strings */
typedef struct {
	ulong arg1;
	ulong arg2;
	ulong arg3;
	ulong arg4;
	ulong arg5;
	ulong arg6;
	ulong arg7;
	ulong arg8;
	char s1[MAX_MSGSTR_LEN + 1];
	char s2[MAX_MSGSTR_LEN + 1];
} sStrMsg;

/* for messages with a few integer arguments and a data-part */
typedef struct {
	ulong arg1;
	ulong arg2;
	ulong arg3;
	ulong arg4;
	ulong arg5;
	ulong arg6;
	ulong arg7;
	ulong arg8;
	char d[MAX_MSG_SIZE];
} sDataMsg;

/* the message we're using for communication; most of the time its nice to have all message-types
 * in one union to be able to use all of them (one after another, of course) */
typedef union {
	sArgsMsg args;
	sStrMsg str;
	sDataMsg data;
} sMsg;
