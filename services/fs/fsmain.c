/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/service.h>
#include <esc/proc.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <string.h>
#include <fsinterface.h>

#include "ext2/ext2.h"
#include "ext2/path.h"
#include "ext2/inode.h"
#include "ext2/inodecache.h"
#include "ext2/file.h"

/* open-response */
typedef struct {
	sMsgHeader header;
	sMsgDataFSOpenResp data;
} __attribute__((packed)) sMsgOpenResp;
/* write-response */
typedef struct {
	sMsgHeader header;
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

int main(void) {
	tFD fd;
	tServ id,client;

	/* TODO */
	ext2.drive = 0;
	ext2.partition = 0;
	if(!ext2_init(&ext2)) {
		unregService(id);
		return 1;
	}

	/* register service */
	id = regService("fs",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printe("Unable to register service 'fs'");
		return 1;
	}

	while(1) {
		fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			sMsgHeader header;
			while(read(fd,&header,sizeof(sMsgHeader)) > 0) {
				switch(header.id) {
					case MSG_FS_OPEN: {
						/* read data */
						sMsgDataFSOpenReq *data = (sMsgDataFSOpenReq*)malloc(sizeof(u8) * header.length);
						if(data != NULL) {
							/* TODO we need a way to skip a message or something.. */
							tInodeNo no;
							read(fd,data,header.length);

							no = ext2_resolvePath(&ext2,data->path);

							/*debugf("Received an open from %d of '%s' for ",data->pid,data + 1);
							if(data->flags & IO_READ)
								debugf("READ");
							if(data->flags & IO_WRITE) {
								if(data->flags & IO_READ)
									debugf(" and ");
								debugf("WRITE");
							}
							debugf("\n");
							debugf("Path is associated with inode %d\n",no);*/

							/*ext2_icache_printStats();
							ext2_bcache_printStats();*/

							/* write response */
							openResp.data.pid = data->pid;
							openResp.data.inodeNo = no;
							write(fd,&openResp,sizeof(sMsgOpenResp));
							free(data);
						}
					}
					break;

					case MSG_FS_READ: {
						sMsgHeader *rhead;
						sMsgDataFSReadResp *rdata;
						u32 dlen;
						u32 count;

						/* read data */
						sMsgDataFSReadReq data;
						read(fd,&data,header.length);

						/* write response  */
						dlen = sizeof(sMsgDataFSReadResp) + data.count * sizeof(u8);
						rhead = (sMsgHeader*)malloc(sizeof(sMsgHeader) + dlen);
						if(rhead != NULL) {
							rdata = (sMsgDataFSReadResp*)(rhead + 1);
							count = ext2_readFile(&ext2,data.inodeNo,rdata->data,
										data.offset,data.count);

							dlen = sizeof(sMsgDataFSReadResp) + count * sizeof(u8);
							rhead->length = dlen;
							rhead->id = MSG_FS_READ_RESP;
							rdata->count = count;
							rdata->pid = data.pid;

							/*ext2_icache_printStats();
							ext2_bcache_printStats();*/

							write(fd,rhead,sizeof(sMsgHeader) + dlen);
							free(rhead);

							/* read ahead
							if(count > 0)
								ext2_readFile(&ext2,data.inodeNo,NULL,data.offset + count,data.count);*/
						}
					}
					break;

					case MSG_FS_WRITE: {
						/* read data */
						sMsgDataFSWriteReq *data = (sMsgDataFSWriteReq*)malloc(header.length);
						if(data != NULL) {
							read(fd,data,header.length);

							debugf("Got '%s' (%d bytes) for offset %d in inode %d\n",data->data,
									data->count,data->offset,data->inodeNo);

							/* write response */
							writeResp.data.pid = data->pid;
							writeResp.data.count = data->count;
							write(fd,&writeResp,sizeof(sMsgWriteResp));
							free(data);
						}
					}
					break;

					case MSG_FS_CLOSE: {
						/* read data */
						sMsgDataFSCloseReq data;
						read(fd,&data,sizeof(sMsgDataFSCloseReq));

						/*debugf("Closing inode %d\n",data.inodeNo);*/
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
