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

/* the message-ids */
#define MSG_KEYBOARD_READ			0
#define MSG_KEYBOARD_DATA			1

#define MSG_VIDEO_SET				20
#define MSG_VIDEO_SETSCREEN			21
#define MSG_VIDEO_SETCURSOR			22

#define MSG_SPEAKER_BEEP			40

#define MSG_ATA_READ_REQ			60
#define MSG_ATA_WRITE_REQ			61
#define MSG_ATA_READ_RESP			62

#define MSG_ENV_GET					80
#define MSG_ENV_SET					81
#define MSG_ENV_GET_RESP			82
#define MSG_ENV_GETI				83

#define MSG_VESA_UPDATE				100
#define MSG_VESA_CURSOR				101
#define MSG_VESA_GETMODE_REQ		102
#define MSG_VESA_GETMODE_RESP		103

#define MSG_MOUSE					120

#define MSG_WIN_CREATE_REQ			140
#define MSG_WIN_CREATE_RESP			141
#define MSG_WIN_MOUSE				142
#define MSG_WIN_MOVE_REQ			143
#define MSG_WIN_UPDATE				144
#define MSG_WIN_KEYBOARD			145
#define MSG_WIN_SET_ACTIVE			146
#define MSG_WIN_DESTROY_REQ			147
#define MSG_WIN_UPDATE_REQ			148

#define MSG_FS_OPEN					160
#define MSG_FS_READ					161
#define MSG_FS_WRITE				162
#define MSG_FS_CLOSE				163
#define MSG_FS_STAT					164

#define MSG_FS_OPEN_RESP			180
#define MSG_FS_READ_RESP			181
#define MSG_FS_WRITE_RESP			182
#define MSG_FS_CLOSE_RESP			183
#define MSG_FS_STAT_RESP			184

#define MSG_RECEIVE					200
#define MSG_SEND					201

/* the header for all default-messages */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgHeader;

/* a message that will be send from the keyboard-service */
typedef struct {
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
} sMsgKbResponse;

/* the message-data for the speaker-beep-message */
typedef struct {
	u16 frequency;
	u16 duration;	/* in ms */
} sMsgDataSpeakerBeep;

/* the message-data for the video-set-message */
typedef struct {
	u8 col;
	u8 row;
	u8 color;
	char character;
} sMsgDataVidSet;

/* the message-data for the video-setcursor-message */
typedef struct {
	u8 col;
	u8 row;
} sMsgDataVidSetCursor;

/* the message-data for a ATA-read- and write-request.
 * A write-request transports the data directly behind this struct */
typedef struct {
	u8 drive;
	u8 partition;
	u64 lba;
	u16 secCount;
} sMsgDataATAReq;

/* the message-data for a VESA-update request */
typedef struct {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
} sMsgDataVesaUpdate;

/* the message-data for mouse-events */
typedef struct {
	s8 x;
	s8 y;
	u8 buttons;
} sMsgDataMouse;

/* the message-data to create a window */
typedef struct {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	u16 tmpWinId;
	tPid owner;
	u8 style;
} sMsgDataWinCreateReq;

/* the message-data to destroy a window */
typedef struct {
	u16 window;
} sMsgDataWinDestroyReq;

/* response-data of creating a window */
typedef struct {
	u16 tmpId;
	u16 id;
} sMsgDataWinCreateResp;

/* mouse-event that the window-manager sends */
typedef struct {
	u16 window;
	u16 x;
	u16 y;
	s16 movedX;
	s16 movedY;
	u8 buttons;
} sMsgDataWinMouse;

/* move-window-request */
typedef struct {
	u16 window;
	u16 x;
	u16 y;
} sMsgDataWinMoveReq;

/* update event */
typedef struct {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	u16 window;
} sMsgDataWinUpdate;

/* sets the cursor */
typedef struct {
	u16 x;
	u16 y;
} sMsgDataVesaCursor;

/* the response of vesa to the get-mode-message */
typedef struct {
	u16 width;
	u16 height;
	u8 colorDepth;
} sMsgDataVesaGetModeResp;

/* the window-manager key-message */
typedef struct {
	/* wether the key was released */
	u8 isBreak;
	/* the keycode (see keycodes.h) */
	u8 keycode;
	char character;
	u8 modifier;
	u16 window;
} sMsgDataWinKeyboard;

/* the window-manager set-active-event */
typedef struct {
	u16 window;
	u8 isActive;
	u16 mouseX;
	u16 mouseY;
} sMsgDataWinActive;

/* the open-request-data */
typedef struct {
	tTid tid;
	/* read/write */
	u8 flags;
	/* pathname follows */
	char path[];
} sMsgDataFSOpenReq;

/* the stat-request-data */
typedef struct {
	tTid tid;
	/* pathname follows */
	char path[];
} sMsgDataFSStatReq;

/* the stat-response-data */
typedef struct {
	tTid tid;
	s32 error;
	sFileInfo info;
} sMsgDataFSStatResp;

/* the open-response-data */
typedef struct {
	tTid tid;
	/* may be an error-code */
	tInodeNo inodeNo;
} sMsgDataFSOpenResp;

/* the read-request-data */
typedef struct {
	tTid tid;
	tInodeNo inodeNo;
	u32 offset;
	u32 count;
} sMsgDataFSReadReq;

/* the read-response-data */
typedef struct {
	tTid tid;
	/* alignment to ensure that we have 2 dwords in front of the data */
	u16 : 16;
	/* may be an error-code */
	s32 count;
	/* data follows */
	u8 data[];
} sMsgDataFSReadResp;

/* the write-request-data */
typedef struct {
	tTid tid;
	tInodeNo inodeNo;
	u32 offset;
	u32 count;
	/* data follows */
	u8 data[];
} sMsgDataFSWriteReq;

/* the write-response-data */
typedef struct {
	tTid tid;
	/* may be an error-code */
	s32 count;
} sMsgDataFSWriteResp;

/* the close-request-data */
typedef struct {
	tInodeNo inodeNo;
} sMsgDataFSCloseReq;

typedef union {
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		u32 arg5;
		u32 arg6;
	} args;
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		char s1[MAX_MSGSTR_LEN];
		char s2[MAX_MSGSTR_LEN];
	} str;
	struct {
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		char d[MAX_MSG_SIZE - sizeof(u32) * 4];
	} data;
} sMsg;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a default-message with the given data.
 * After you've sent the message, please use freeMsg() to free the space on the heap.
 *
 * @param id the message-id
 * @param length the length of the data to send
 * @param buf the data to send
 * @return the message or NULL if failed
 */
sMsgHeader *asmDataMsg(u8 id,u32 length,const void *data);

/**
 * Assembles a binary message with the given arguments. <fmt> specifies the size of the arguments
 * in bytes. For example:
 * asmBinMsg(0,"142",0xFF,0xFFFFFFFF,0xFFFF);
 * After you've sent the message, please use freeMsg() to free the space on the heap.
 *
 * @param id the message-id
 * @param fmt the format of the arguments
 * @return the message or NULL if failed
 */
sMsgHeader *asmBinMsg(u8 id,const char *fmt,...);

/**
 * Assembles a binary message with the given arguments and puts the given data behind them.
 * <fmt> specifies the size of the arguments in bytes. For example:
 * asmBinDataMsg(0,"MYSTRING",strlen("MYSTRING") + 1,"142",0xFF,0xFFFFFFFF,0xFFFF);
 * After you've sent the message, please use freeMsg() to free the space on the heap.
 *
 * @param id the message-id
 * @param data the data to append to the message
 * @param dataLen the number of bytes of the data
 * @param fmt the format of the arguments
 * @return the message or NULL if failed
 */
sMsgHeader *asmBinDataMsg(u8 id,const void *data,u32 dataLen,const char *fmt,...);

/**
 * Disassembles the given binary-message. <fmt> specifies the size of the arguments
 * in bytes. For example:
 * disasmBinMsg(data,"142",&byte,&dword,&word);
 *
 * @param data the message-data
 * @param fmt the format of the arguments
 * @return true if successfull
 */
bool disasmBinMsg(const void *data,const char *fmt,...);

/**
 * Disassembles the given binary-message with data appended. <fmt> specifies the size of the arguments
 * in bytes. For example:
 * disasmBinDataMsg(5,data,&buffer,"142",&byte,&dword,&word);
 *
 * @param msgLen the length of the whole message (without header)
 * @param data the message-data
 * @param buffer will point to the appended data after the call
 * @param fmt the format of the arguments
 * @return the number of bytes of the appended data or 0 if failed
 */
u32 disasmBinDataMsg(u32 msgLen,const void *data,u8 **buffer,const char *fmt,...);

/**
 * Free's the given message
 *
 * @param msg the message
 */
void freeMsg(sMsgHeader *msg);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGES_H_ */
