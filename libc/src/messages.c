/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/heap.h"
#include "../h/string.h"

sConsoleMsg *createConsoleMsg(u8 id,u32 length,void *buf)
{
	sConsoleMsg *msg = (sConsoleMsg*)malloc(sizeof(sConsoleMsg) + length * sizeof(u8));
	if(msg == NULL)
		return NULL;

	msg->id = id;
	msg->length = length;
	if(length > 0)
		memcpy(msg + 1,buf,length);
	return msg;
}

void freeConsoleMsg(sConsoleMsg *msg) {
	free(msg);
}
