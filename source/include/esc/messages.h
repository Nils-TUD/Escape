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

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <stddef.h>

/* general */
#define MAX_MSG_ARGS				10
#define MAX_MSG_SIZE				128
#define MAX_MSGSTR_LEN				63

/* cursors */
#define CURSOR_DEFAULT				0
#define CURSOR_RESIZE_L				1
#define CURSOR_RESIZE_BR			2
#define CURSOR_RESIZE_VERT			3
#define CURSOR_RESIZE_BL			4
#define CURSOR_RESIZE_R				5
#define CURSOR_RESIZE_WIDTH			6

/* the possible km-events to listen to; KE_EV_PRESSED, KE_EV_RELEASED and KE_EV_KEYCODE,
 * KE_EV_CHARACTER are mutually exclusive, each */
#define KE_EV_PRESSED				1
#define KE_EV_RELEASED				2
#define KE_EV_KEYCODE				4
#define KE_EV_CHARACTER				8

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
#define MSG_FS_CHMOD_RESP			12
#define MSG_FS_CHOWN_RESP			13
/* device */
#define MSG_DEV_OPEN_RESP			14
#define MSG_DEV_READ_RESP			15
#define MSG_DEV_WRITE_RESP			16
#define MSG_DEV_CLOSE_RESP			17
/* default response */
#define MSG_DEF_RESPONSE			18

/* requests to device */
#define MSG_DEV_OPEN				50
#define MSG_DEV_READ				51
#define MSG_DEV_WRITE				52
#define MSG_DEV_CLOSE				53

/* requests to fs */
#define MSG_FS_OPEN					54
#define MSG_FS_READ					55
#define MSG_FS_WRITE				56
#define MSG_FS_CLOSE				57
#define MSG_FS_STAT					58
#define MSG_FS_SYNC					59
#define MSG_FS_LINK					60
#define MSG_FS_UNLINK				61
#define MSG_FS_MKDIR				62
#define MSG_FS_RMDIR				63
#define MSG_FS_MOUNT				64
#define MSG_FS_UNMOUNT				65
#define MSG_FS_ISTAT				66
#define MSG_FS_CHMOD				67
#define MSG_FS_CHOWN				68

/* == Other messages == */
#define MSG_SPEAKER_BEEP			100	/* performs a beep */

#define MSG_VESA_UPDATE				200	/* updates a region of the screen */
#define MSG_VESA_CURSOR				201	/* sets the cursor */
#define MSG_VESA_GETMODE			202	/* gets the vesa-mode */
#define MSG_VESA_GETMODE_RESP		203	/* response for the get-mode-request */
#define MSG_VESA_SETMODE			204	/* sets the vesa-mode */
#define MSG_VESA_ENABLE				205	/* enables vesa */
#define MSG_VESA_DISABLE			206	/* disables vesa */
#define MSG_VESA_PREVIEWRECT		207 /* sets the preview-rectangle */

#define MSG_WIN_CREATE				300	/* creates a window */
#define MSG_WIN_CREATE_RESP			301	/* the create-response */
#define MSG_WIN_MOVE				302	/* moves a window */
#define MSG_WIN_UPDATE				303	/* requests an update of a window */
#define MSG_WIN_SET_ACTIVE_EV		304	/* sets the active window */
#define MSG_WIN_DESTROY				305	/* destroys a window */
#define MSG_WIN_RESIZE				306	/* resizes a window */
#define MSG_WIN_ENABLE				307	/* enables the window-manager */
#define MSG_WIN_DISABLE				308	/* disables the window-manager */
#define MSG_WIN_UPDATE_EV			309	/* is sent by the window-manager to a window to do an update */
#define MSG_WIN_MOUSE_EV			310	/* a mouse-event sent by the window-manager */
#define MSG_WIN_KEYBOARD_EV			311	/* a keyboard-event sent by the window-manager */
#define MSG_WIN_CREATE_EV			312	/* a window has been created */
#define MSG_WIN_DESTROY_EV			313	/* a window has been destroyed */
#define MSG_WIN_ADDLISTENER			314	/* announces a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_REMLISTENER			315	/* removes a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_SET_ACTIVE			316	/* requests that a window is set to the active one */
#define MSG_WIN_ACTIVE_EV			317	/* a window has been set to active */

#define MSG_VID_SETCURSOR			500	/* expects sVTPos */
#define MSG_VID_GETSIZE				501	/* writes into sVTSize */
#define MSG_VID_SETMODE				502	/* sets the video-mode */

#define MSG_VT_EN_ECHO				600	/* enables that the vterm echo's typed characters */
#define MSG_VT_DIS_ECHO				601	/* disables echo */
#define MSG_VT_EN_RDLINE			602	/* enables the readline-feature */
#define MSG_VT_DIS_RDLINE			603	/* disables the readline-feature */
#define MSG_VT_EN_RDKB				604	/* allows vterm to read from keyboard */
#define MSG_VT_DIS_RDKB				605	/* prohibits vterm to read from keyboard */
#define MSG_VT_EN_NAVI				606	/* enables navigation (page-up, arrow-up, ... ) */
#define MSG_VT_DIS_NAVI				607	/* disables navigation */
#define MSG_VT_BACKUP				608	/* backups the screen */
#define MSG_VT_RESTORE				609	/* restores the screen */
#define MSG_VT_SHELLPID				610	/* gives the vterm the shell-pid */
#define MSG_VT_GETSIZE				611	/* writes into sVTSize */
#define MSG_VT_ENABLE				612	/* enables vterm */
#define MSG_VT_DISABLE				613	/* disables vterm */
#define MSG_VT_SELECT				614	/* selects the vterm */

#define MSG_KM_SET					700	/* sets a keymap, expects the keymap-path as argument */
#define MSG_KM_EVENT				701	/* the message-id for sending events to the listeners */

#define MSG_KE_ADDLISTENER			800	/* adds a listener (see KE_EV_*) */
#define MSG_KE_REMLISTENER			801	/* removes a listener */

#define MSG_PCI_GET_BY_CLASS		900	/* searches for a pci device with given class */
#define MSG_PCI_GET_BY_ID			901	/* searches for a pci device with given id */
#define MSG_PCI_DEVICE_RESP			902

#define MSG_INIT_REBOOT				1000 /* requests a reboot */
#define MSG_INIT_SHUTDOWN			1001 /* requests a shutdown */
#define MSG_INIT_IAMALIVE			1002 /* tells init that the shutdown-request has been received
											and that you promise to terminate as soon as possible */

#define IS_DEVICE_MSG(id)			((id) == MSG_DEV_OPEN || \
									 (id) == MSG_DEV_READ || \
									 (id) == MSG_DEV_WRITE || \
									 (id) == MSG_DEV_CLOSE)

/* the data read from the keyboard */
typedef struct {
	/* the keycode (see keycodes.h) */
	uchar keycode;
	/* whether the key was released */
	uchar isBreak;
} sKbData;

typedef struct {
	/* the keycode (see keycodes.h) */
	uchar keycode;
	/* modifiers (STATE_CTRL, STATE_SHIFT, STATE_ALT, STATE_BREAK) */
	uchar modifier;
	/* the character, translated by the current keymap */
	char character;
} sKmData;

/* the data read from the mouse */
typedef struct {
	gpos_t x;
	gpos_t y;
	gpos_t z;
	uchar buttons;
} sMouseData;

typedef struct {
	gsize_t width;					/* x-resolution */
	gsize_t height;					/* y-resolution */
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

#endif /* MESSAGES_H_ */
