/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <io.h>
#include <heap.h>
#include <debug.h>
#include <proc.h>
#include "ata.h"

typedef struct {
	sMsgDefHeader header;
	sMsgDataATAReq data;
} __attribute__((packed)) sMsgATAReq;

s32 shell_cmdAta(u32 argc,s8 **argv) {
	UNUSED(argc);
	UNUSED(argv);

	tFD fd;
	sMsgATAReq req = {
		.header = {
			.id = MSG_ATA_READ_REQ,
			.length = sizeof(sMsgDataATAReq)
		},
		.data = {
			.drive = 0,
			.lba = 0,
			.secCount = 4
		}
	};
	sMsgDefHeader res;
	u16 *buffer;

	fd = open("services:/ata",IO_WRITE | IO_READ);
	write(fd,&req,sizeof(sMsgATAReq));
	/* wait for response */
	do {
		sleep();
	}
	while(read(fd,&res,sizeof(sMsgDefHeader)) <= 0);

	/* read response */
	buffer = (u16*)malloc(sizeof(u16) * res.length);
	read(fd,buffer,res.length);
	dumpBytes(buffer,res.length);

	free(buffer);
	close(fd);
	return 0;
}
