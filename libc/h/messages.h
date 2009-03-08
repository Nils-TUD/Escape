/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "common.h"

#define MSG_CONSOLE_OUT		0
#define MSG_CONSOLE_IN		1
#define MSG_CONSOLE_CLEAR	2

#define MSG_KEYBOARD_READ	0

#define MSG_VIDEO_PUTCHAR	0
#define MSG_VIDEO_PUTS		1
#define MSG_VIDEO_GOTO		2

/* a message that can be send to the console-service */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgConRequest;

/* a request-message for the keyboard-service */
typedef struct {
	/* the message-id */
	u8 id;
} sMsgKbRequest;

/* a message that will be send from the keyboard-service */
typedef struct {
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
} sMsgKbResponse;

/* a message that can be send to the video-service */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgVidRequest;

/* the message-data for the video-GOTO-message */
typedef struct {
	u8 col;
	u8 row;
} sMsgDataVidGoto;

/**
 * Creates a console-message with the given data
 *
 * @param id the message-id
 * @param length the length of the data to send
 * @param buf the data to send
 * @return the message or NULL if failed
 */
sMsgConRequest *createConsoleMsg(u8 id,u32 length,void *buf);

/**
 * Frees the messages that has been build with createConsoleMsg().
 *
 * @param msg the message
 */
void freeConsoleMsg(sMsgConRequest *msg);

#endif /* MESSAGES_H_ */
