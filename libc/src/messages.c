/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/heap.h"
#include "../h/string.h"

sMsgDefHeader *createDefMsg(u8 id,u32 length,void *buf)
{
	sMsgDefHeader *msg = (sMsgDefHeader*)malloc(sizeof(sMsgDefHeader) + length * sizeof(u8));
	if(msg == NULL)
		return NULL;

	msg->id = id;
	msg->length = length;
	if(length > 0)
		memcpy(msg + 1,buf,length);
	return msg;
}

void freeDefMsg(sMsgDefHeader *msg) {
	free(msg);
}
