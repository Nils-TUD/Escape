/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/heap.h"
#include "../h/string.h"
#include <stdarg.h>

/**
 * Internal function to build a bin-message
 *
 * @param id the message-id
 * @param dataLen the number of bytes to append behind the given parameter
 * @param fmt the format
 * @param ap the argument-list
 * @return the msg or NULL
 */
static sMsgHeader *doAsmBinMsg(u8 id,u32 dataLen,const s8 *fmt,va_list ap);

/**
 * Internal function to disassemble a bin-message
 *
 * @param data the message-data
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of read bytes
 */
static u32 doDisasmBinMsg(void *data,const s8 *fmt,va_list ap);

sMsgHeader *asmDataMsg(u8 id,u32 length,void *data) {
	sMsgHeader *msg = (sMsgHeader*)malloc(sizeof(sMsgHeader) + length * sizeof(u8));
	if(msg == NULL)
		return NULL;

	msg->id = id;
	msg->length = length;
	if(length > 0)
		memcpy(msg + 1,data,length);
	return msg;
}

sMsgHeader *asmBinMsg(u8 id,const s8 *fmt,...) {
	sMsgHeader *msg;
	va_list ap;
	va_start(ap,fmt);
	msg = doAsmBinMsg(id,0,fmt,ap);
	va_end(ap);
	return msg;
}

sMsgHeader *asmBinDataMsg(u8 id,void *data,u32 dataLen,const s8 *fmt,...) {
	sMsgHeader *msg;
	u8 *dPtr;
	va_list ap;

	/* build msg */
	va_start(ap,fmt);
	msg = doAsmBinMsg(id,dataLen,fmt,ap);
	va_end(ap);

	/* append data */
	if(msg != NULL) {
		dPtr = (u8*)msg + sizeof(sMsgHeader) + (msg->length - dataLen);
		memcpy(dPtr,data,dataLen);
	}
	return msg;
}

static sMsgHeader *doAsmBinMsg(u8 id,u32 dataLen,const s8 *fmt,va_list ap) {
	s8 c;
	s8 *str;
	u8 *data;
	u32 msgSize;
	sMsgHeader *msg;

	/* determine message-size */
	msgSize = dataLen;
	str = (s8*)fmt;
	while((c = *str)) {
		switch(c) {
			case '1':
			case '2':
			case '4':
				msgSize += c - '0';
				break;
			default:
				return NULL;
		}
		str++;
	}

	/* alloc mem */
	msg = (sMsgHeader*)malloc(sizeof(sMsgHeader) + msgSize);
	if(msg == NULL)
		return NULL;

	/* set header */
	msg->id = id;
	msg->length = msgSize;
	data = (u8*)(msg + 1);

	/* copy data */
	while((c = *fmt)) {
		switch(c) {
			case '1':
				*data = (u8)va_arg(ap,u32);
				break;
			case '2':
				*((u16*)data) = (u16)va_arg(ap,u32);
				break;
			case '4':
				*((u32*)data) = va_arg(ap,u32);
				break;
		}
		data += c - '0';
		fmt++;
	}

	return msg;
}

bool disasmBinMsg(void *data,const s8 *fmt,...) {
	va_list ap;
	bool res;
	va_start(ap,fmt);
	res = doDisasmBinMsg(data,fmt,ap);
	va_end(ap);
	return res;
}

u32 disasmBinDataMsg(u32 msgLen,void *data,u8 **buffer,const s8 *fmt,...) {
	u32 res;
	va_list ap;

	/* disassemble message */
	va_start(ap,fmt);
	res = doDisasmBinMsg(data,fmt,ap);
	va_end(ap);

	/* copy data */
	if(res > 0) {
		*buffer = (u8*)data + res;
		return msgLen - res;
	}
	return 0;
}

static u32 doDisasmBinMsg(void *data,const s8 *fmt,va_list ap) {
	s8 c;
	u8 *ptr8;
	u16 *ptr16;
	u32 *ptr32;
	u8 *d,*dataBak;

	dataBak = d = data;
	while((c = *fmt)) {
		switch(c) {
			case '1':
				ptr8 = va_arg(ap,u8*);
				*ptr8 = *(u8*)d;
				break;
			case '2':
				ptr16 = va_arg(ap,u16*);
				*ptr16 = *(u16*)d;
				break;
			case '4':
				ptr32 = va_arg(ap,u32*);
				*ptr32 = *(u32*)d;
				break;
			default:
				return 0;
		}
		d += c - '0';
		fmt++;
	}
	return (u8*)d - dataBak;
}

void freeMsg(sMsgHeader *msg) {
	free(msg);
}
