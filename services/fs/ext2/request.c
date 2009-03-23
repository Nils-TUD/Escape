/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <io.h>
#include <proc.h>
#include "request.h"
#include "ext2.h"

/* the ATA-request-message */
typedef struct {
	sMsgHeader header;
	sMsgDataATAReq data;
} __attribute__((packed)) sMsgATAReq;

static sMsgATAReq req = {
	.header = {
		.id = MSG_ATA_READ_REQ,
		.length = sizeof(sMsgDataATAReq)
	},
	.data = {
		.drive = 0,
		.partition = 0,
		.lba = 0,
		.secCount = 0
	}
};

bool ext2_readBlocks(sExt2 *e,u8 *buffer,u32 start,u16 blockCount) {
	return ext2_readSectors(e,buffer,BLOCKS_TO_SECS(e,start),BLOCKS_TO_SECS(e,blockCount));
}

bool ext2_readSectors(sExt2 *e,u8 *buffer,u64 lba,u16 secCount) {
	sMsgHeader res;

	/* send read-request */
	req.data.drive = e->drive;
	req.data.partition = e->partition;
	req.data.lba = lba;
	req.data.secCount = secCount;
	if(write(e->ataFd,&req,sizeof(sMsgATAReq)) < 0) {
		printLastError();
		return false;
	}

	/* read header */
	if(read(e->ataFd,&res,sizeof(sMsgHeader)) < 0) {
		printLastError();
		return false;
	}

	/* read response */
	if(read(e->ataFd,buffer,res.length) < 0) {
		printLastError();
		return false;
	}

	return true;
}
