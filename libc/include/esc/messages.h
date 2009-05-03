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

#include <esc/common.h>

#define MAX_MSG_ARGS				10

#define MSG_KEYBOARD_READ			0

#define MSG_VIDEO_SET				0
#define MSG_VIDEO_SETSCREEN			1
#define MSG_VIDEO_SETCURSOR			2

#define MSG_SPEAKER_BEEP			0

#define MSG_ATA_READ_REQ			0
#define MSG_ATA_WRITE_REQ			1
#define MSG_ATA_READ_RESP			2

#define MSG_ENV_GET					0
#define MSG_ENV_SET					1
#define MSG_ENV_GET_RESP			2
#define MSG_ENV_GETI				3

#define MSG_VESA_UPDATE				0
#define MSG_VESA_CURSOR				1
#define MSG_VESA_GETMODE_REQ		2
#define MSG_VESA_GETMODE_RESP		3

#define MSG_MOUSE					0

#define MSG_WIN_CREATE_REQ			0
#define MSG_WIN_CREATE_RESP			1
#define MSG_WIN_MOUSE				2
#define MSG_WIN_MOVE_REQ			3
#define MSG_WIN_REPAINT				4
#define MSG_WIN_KEYBOARD			5

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
	tPid owner;
} sMsgDataWinCreateReq;

/* response-data of creating a window */
typedef struct {
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

/* repaint event */
typedef struct {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	u16 window;
} sMsgDataWinRepaint;

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
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
	u16 window;
} sMsgDataWinKeyboard;

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
