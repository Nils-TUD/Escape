/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <service.h>
#include <proc.h>
#include <heap.h>
#include <debug.h>
#include <messages.h>
#include <string.h>

#include "ext2/ext2.h"
#include <fsinterface.h>

/* open-response */
typedef struct {
	sMsgDefHeader header;
	sMsgDataFSOpenResp data;
} __attribute__((packed)) sMsgOpenResp;
/* write-response */
typedef struct {
	sMsgDefHeader header;
	sMsgDataFSWriteResp data;
} __attribute__((packed)) sMsgWriteResp;

/* the message we'll send */
static sMsgOpenResp openResp = {
	.header = {
		.id = MSG_FS_OPEN_RESP,
		.length = sizeof(sMsgDataFSOpenResp)
	},
	.data = {
		.pid = 0,
		.inodeNo = 0
	}
};
static sMsgWriteResp writeResp = {
	.header = {
		.id = MSG_FS_WRITE_RESP,
		.length = sizeof(sMsgDataFSWriteResp)
	},
	.data = {
		.pid = 0,
		.count = 0
	}
};

static sExt2 ext2;

s32 main(void) {
	s32 fd,id;

	/* register service */
	id = regService("fs",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* TODO */
	ext2.drive = 0;
	ext2.partition = 0;
	if(!ext2_init(&ext2)) {
		unregService(id);
		return 1;
	}

	while(1) {
		fd = getClient(id);
		if(fd < 0)
			sleep();
		else {
			sMsgDefHeader header;
			while(read(fd,&header,sizeof(sMsgDefHeader)) > 0) {
				switch(header.id) {
					case MSG_FS_OPEN: {
						/* read data */
						sMsgDataFSOpenReq data;
						read(fd,&data,header.length);

						ext2_dbg_printInode(ext2_resolvePath(&ext2,&data + 1));

						debugf("Received an open from %d of '%s' for ",data.pid,&data + 1);
						if(data.flags & IO_READ)
							debugf("READ");
						if(data.flags & IO_WRITE) {
							if(data.flags & IO_READ)
								debugf(" and ");
							debugf("WRITE");
						}
						debugf("\n");

						/* write response */
						openResp.data.pid = data.pid;
						openResp.data.inodeNo = 0x1337;
						write(fd,&openResp,sizeof(sMsgOpenResp));
					}
					break;

					case MSG_FS_READ: {
						/* read data */
						sMsgDataFSReadReq data;
						read(fd,&data,header.length);

						/* write response */
						u32 dlen = sizeof(sMsgDataFSReadResp) + 5 * sizeof(s8);
						sMsgDefHeader *rhead = malloc(sizeof(sMsgDefHeader) + dlen);
						rhead->id = MSG_FS_READ_RESP;
						rhead->length = dlen;
						sMsgDataFSReadResp *rdata = (sMsgDataFSReadResp*)(rhead + 1);
						rdata->count = 5;
						rdata->pid = data.pid;
						s8 *str = (s8*)(rdata + 1);
						memcpy(str,"test",5);

						write(fd,rhead,sizeof(sMsgDefHeader) + dlen);
						free(rhead);
					}
					break;

					case MSG_FS_WRITE: {
						/* read data */
						sMsgDataFSWriteReq *data = malloc(header.length);
						read(fd,data,header.length);

						debugf("Got '%s' (%d bytes) for offset %d in inode %d\n",data + 1,
								data->count,data->offset,data->inodeNo);

						/* write response */
						writeResp.data.pid = data->pid;
						writeResp.data.count = data->count;
						write(fd,&writeResp,sizeof(sMsgWriteResp));
					}
					break;

					case MSG_FS_CLOSE: {
						/* read data */
						sMsgDataFSCloseReq data;
						read(fd,&data,sizeof(sMsgDataFSCloseReq));

						debugf("Closing inode %d\n",data.inodeNo);
					}
					break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	unregService(id);

	return 0;
}
