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
	sMsgDefHeader header;
	sMsgDataATAReq data;
} __attribute__((packed)) sMsgATAReq;

bool ext2_readBlocks(sExt2 *e,u8 *buffer,u32 start,u16 blockCount) {
	return ext2_readSectors(e,buffer,BLOCKS_TO_SECS(e,start),BLOCKS_TO_SECS(e,blockCount));
}

bool ext2_readSectors(sExt2 *e,u8 *buffer,u64 lba,u16 secCount) {
	sMsgATAReq req = {
		.header = {
			.id = MSG_ATA_READ_REQ,
			.length = sizeof(sMsgDataATAReq)
		},
		.data = {
			.drive = e->drive,
			.partition = e->partition,
			.lba = lba,
			.secCount = secCount
		}
	};
	sMsgDefHeader res;

	/* send read-request */
	if(write(e->ataFd,&req,sizeof(sMsgATAReq)) < 0) {
		printLastError();
		return false;
	}

	/* wait for response */
	do {
		sleep();
	}
	while(read(e->ataFd,&res,sizeof(sMsgDefHeader)) <= 0);

	/* read response */
	if(read(e->ataFd,buffer,res.length) < 0) {
		printLastError();
		return false;
	}

	return true;
}
