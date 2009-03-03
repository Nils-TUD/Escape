/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "common.h"

#define CONSOLE_MSG_OUT		0
#define CONSOLE_MSG_IN		1
#define CONSOLE_MSG_CLEAR	2

/* a message that can be send to the console-service */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sConsoleMsg;

/**
 * Creates a console-message with the given data
 *
 * @param id the message-id
 * @param length the length of the data to send
 * @param buf the data to send
 * @return the message or NULL if failed
 */
sConsoleMsg *createConsoleMsg(u8 id,u32 length,void *buf);

/**
 * Frees the messages that has been build with createConsoleMsg().
 *
 * @param msg the message
 */
void freeConsoleMsg(sConsoleMsg *msg);

#endif /* MESSAGES_H_ */
