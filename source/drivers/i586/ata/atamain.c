/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <usergroup/group.h>
#include <esc/arch/i586/ports.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/mem.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ata.h"
#include "controller.h"
#include "device.h"
#include "partition.h"

#define MAX_RW_SIZE		4096
#define RETRY_COUNT		3
#define MAX_CLIENT_FD	256

typedef struct {
	int fd;
	uint device;
	uint partition;
} sDrive;

static ulong handleRead(sATADevice *device,sPartition *part,uint16_t *buf,uint offset,uint count);
static ulong handleWrite(sATADevice *device,sPartition *part,int fd,uint16_t *buf,uint offset,uint count);
static void initDrives(void);
static void createVFSEntry(sATADevice *device,sPartition *part,const char *name);

static size_t drvCount = 0;
static sDrive drives[DEVICE_COUNT * PARTITION_COUNT];
/* don't use dynamic memory here since this may cause trouble with swapping (which we do) */
/* because if the heap hasn't enough memory and we request more when we should swap the kernel
 * may not have more memory and can't do anything about it */
static uint16_t buffer[MAX_RW_SIZE / sizeof(uint16_t)];
static char *shbufs[MAX_CLIENT_FD];

static int drive_thread(void *arg) {
	sMsg msg;
	sDrive *dev = (sDrive*)arg;
	sATADevice *ataDev = ctrl_getDevice(dev->device);
	sPartition *part = ataDev ? ataDev->partTable + dev->partition : NULL;
	if(ataDev == NULL || part == NULL || ataDev->present == 0 || part->present == 0)
		error("Invalid device/partition");

	while(1) {
		msgid_t mid;
		int fd = getwork(dev->fd,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get client");
		}
		else {
			ATA_PR2("Got message %d",mid);
			switch(mid) {
				case MSG_DEV_OPEN: {
					msg.args.arg1 = fd < MAX_CLIENT_FD ? 0 : -ENOMEM;
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_SHFILE: {
					size_t size = msg.args.arg1;
					char *path = msg.str.s1;
					assert(shbufs[fd] == NULL);
					/* ensure that we don't cause pagefaults when accessing this memory. therefore,
					 * we populate it immediately and lock it into memory. additionally, we specify
					 * MAP_NOSWAP to let it fail if there is not enough memory instead of starting
					 * to swap (which would cause a deadlock, because we're doing that). */
					shbufs[fd] = joinbuf(path,size,MAP_POPULATE | MAP_NOSWAP | MAP_LOCKED);
					msg.args.arg1 = shbufs[fd] != NULL ? 0 : -errno;
					send(fd,MSG_DEV_SHFILE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					uint16_t *buf = shmemoff == -1 ? buffer : (uint16_t*)shbufs[fd] + (shmemoff >> 1);
					msg.args.arg1 = handleRead(ataDev,part,buf,offset,count);
					msg.args.arg2 = READABLE_DONT_SET;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(shmemoff == -1 && msg.args.arg1 > 0)
						send(fd,MSG_DEV_READ_RESP,buf,count);
				}
				break;

				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					uint16_t *buf = shmemoff == -1 ? buffer : (uint16_t*)shbufs[fd] + (shmemoff >> 1);
					msg.args.arg1 = handleWrite(ataDev,part,fd,buf,offset,count);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DISK_GETSIZE: {
					msg.args.arg1 = part->size * ataDev->secSize;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					if(shbufs[fd]) {
						munmap(shbufs[fd]);
						shbufs[fd] = NULL;
					}
					close(fd);
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			ATA_PR2("Done\n");
		}
	}
}

int main(int argc,char **argv) {
	size_t i;
	bool useDma = true;
	FILE *f;

	if(argc < 2) {
		printe("Usage: %s <wait> [nodma]",argv[0]);
		return EXIT_FAILURE;
	}

	for(i = 2; (int)i < argc; i++) {
		if(strcmp(argv[i],"nodma") == 0)
			useDma = false;
	}

	/* detect and init all devices */
	ctrl_init(useDma);
	initDrives();
	/* flush prints */
	fflush(stdout);

	/* we're ready now, so create a dummy-vfs-node that tells fs that all ata-devices are registered */
	f = fopen("/system/devices/ata","w");
	fclose(f);

	/* start drive threads */
	for(i = 1; i < drvCount; i++) {
		if(startthread(drive_thread, drives + i) < 0)
			error("Unable to start thread");
	}

	/* mlock all regions to prevent that we're swapped out */
	if(mlockall() < 0)
		error("Unable to mlock regions");

	drive_thread(drives + 0);

	/* clean up */
	relports(ATA_REG_BASE_PRIMARY,8);
	relports(ATA_REG_BASE_SECONDARY,8);
	relport(ATA_REG_BASE_PRIMARY + ATA_REG_CONTROL);
	relport(ATA_REG_BASE_SECONDARY + ATA_REG_CONTROL);
	for(i = 0; i < drvCount; i++)
		close(drives[i].fd);
	return EXIT_SUCCESS;
}

static ulong handleRead(sATADevice *ataDev,sPartition *part,uint16_t *buf,uint offset,uint count) {
	/* we have to check whether it is at least one sector. otherwise ATA can't
	 * handle the request */
	if(offset + count <= part->size * ataDev->secSize && offset + count > offset) {
		uint rcount = ROUND_UP(count,ataDev->secSize);
		if(buf != buffer || rcount <= MAX_RW_SIZE) {
			size_t i;
			ATA_PR2("Reading %d bytes @ %x from device %d",
					rcount,offset,ataDev->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Read failed; retry %zu",i);
				if(ataDev->rwHandler(ataDev,OP_READ,buf,
						offset / ataDev->secSize + part->start,
						ataDev->secSize,rcount / ataDev->secSize)) {
					return count;
				}
			}
			ATA_LOG("Giving up after %zu retries",i);
			return 0;
		}
	}
	ATA_LOG("Invalid read-request: offset=%u, count=%u, partSize=%u (device %d)",
			offset,count,part->size * ataDev->secSize,ataDev->id);
	return 0;
}

static ulong handleWrite(sATADevice *ataDev,sPartition *part,int fd,uint16_t *buf,uint offset,uint count) {
	msgid_t mid;
	if(offset + count <= part->size * ataDev->secSize && offset + count > offset) {
		if(buf != buffer || count <= MAX_RW_SIZE) {
			size_t i;
			if(buf == buffer) {
				ssize_t res = IGNSIGS(receive(fd,&mid,buffer,count));
				if(res <= 0)
					return res;
			}

			ATA_PR2("Writing %d bytes @ %x to device %d",count,offset,ataDev->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Write failed; retry %zu",i);
				if(ataDev->rwHandler(ataDev,OP_WRITE,buf,
						offset / ataDev->secSize + part->start,
						ataDev->secSize,count / ataDev->secSize)) {
					return count;
				}
			}
			ATA_LOG("Giving up after %zu retries",i);
			return 0;
		}
	}
	ATA_LOG("Invalid write-request: offset=%u, count=%u, partSize=%u (device %d)",
			offset,count,part->size * ataDev->secSize,ataDev->id);
	return 0;
}

static void initDrives(void) {
	uint deviceIds[] = {DEVICE_PRIM_MASTER,DEVICE_PRIM_SLAVE,DEVICE_SEC_MASTER,DEVICE_SEC_SLAVE};
	char name[SSTRLEN("hda1") + 1];
	char path[MAX_PATH_LEN] = "/dev/";
	size_t i,p;
	for(i = 0; i < DEVICE_COUNT; i++) {
		sATADevice *ataDev = ctrl_getDevice(deviceIds[i]);
		if(ataDev->present == 0)
			continue;

		/* build VFS-entry */
		if(ataDev->info.general.isATAPI)
			snprintf(name,sizeof(name),"cd%c",'a' + ataDev->id);
		else
			snprintf(name,sizeof(name),"hd%c",'a' + ataDev->id);
		createVFSEntry(ataDev,NULL,name);

		/* register device for every partition */
		for(p = 0; p < PARTITION_COUNT; p++) {
			if(ataDev->partTable[p].present) {
				if(!ataDev->info.general.isATAPI)
					snprintf(name,sizeof(name),"hd%c%d",'a' + ataDev->id,p + 1);
				else
					snprintf(name,sizeof(name),"cd%c%d",'a' + ataDev->id,p + 1);
				strcpy(path + SSTRLEN("/dev/"),name);
				int fd = createdev(path,0770,DEV_TYPE_BLOCK,
					DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_CLOSE);
				if(fd < 0) {
					ATA_LOG("Drive %d, Partition %d: Unable to register device '%s'",
							ataDev->id,p + 1,name);
				}
				else {
					ATA_LOG("Registered device '%s' (device %d, partition %d)",
							name,ataDev->id,p + 1);
					/* set group */
					strcpy(path + SSTRLEN("/dev/"),name);
					if(chown(path,-1,GROUP_STORAGE) < 0)
						ATA_LOG("Unable to set group for '%s'",path);
					createVFSEntry(ataDev,ataDev->partTable + p,name);
					drives[drvCount].device = ataDev->id;
					drives[drvCount].partition = p;
					drives[drvCount].fd = fd;
					drvCount++;
				}
			}
		}
	}
}

static void createVFSEntry(sATADevice *ataDev,sPartition *part,const char *name) {
	FILE *f;
	char path[SSTRLEN("/system/devices/hda1") + 1];
	snprintf(path,sizeof(path),"/system/devices/%s",name);

	/* open and create file */
	f = fopen(path,"w");
	if(f == NULL) {
		printe("Unable to open '%s'",path);
		return;
	}

	if(part == NULL) {
		size_t i;
		fprintf(f,"%-15s%s\n","Type:",ataDev->info.general.isATAPI ? "ATAPI" : "ATA");
		fprintf(f,"%-15s","ModelNo:");
		for(i = 0; i < 40; i += 2)
			fprintf(f,"%c%c",ataDev->info.modelNo[i + 1],ataDev->info.modelNo[i]);
		fprintf(f,"\n");
		fprintf(f,"%-15s","SerialNo:");
		for(i = 0; i < 20; i += 2)
			fprintf(f,"%c%c",ataDev->info.serialNumber[i + 1],ataDev->info.serialNumber[i]);
		fprintf(f,"\n");
		if(ataDev->info.firmwareRev[0] && ataDev->info.firmwareRev[1]) {
			fprintf(f,"%-15s","FirmwareRev:");
			for(i = 0; i < 8; i += 2)
				fprintf(f,"%c%c",ataDev->info.firmwareRev[i + 1],ataDev->info.firmwareRev[i]);
			fprintf(f,"\n");
		}
		fprintf(f,"%-15s0x%02x\n","MajorVersion:",ataDev->info.majorVersion.raw);
		fprintf(f,"%-15s0x%02x\n","MinorVersion:",ataDev->info.minorVersion);
		fprintf(f,"%-15s%d\n","Sectors:",ataDev->info.userSectorCount);
		if(ataDev->info.words5458Valid) {
			fprintf(f,"%-15s%d\n","Cylinder:",ataDev->info.oldCylinderCount);
			fprintf(f,"%-15s%d\n","Heads:",ataDev->info.oldHeadCount);
			fprintf(f,"%-15s%d\n","SecsPerTrack:",ataDev->info.oldSecsPerTrack);
		}
		fprintf(f,"%-15s%d\n","LBA:",ataDev->info.capabilities.LBA);
		fprintf(f,"%-15s%d\n","LBA48:",ataDev->info.features.lba48);
		fprintf(f,"%-15s%d\n","DMA:",ataDev->info.capabilities.DMA);
	}
	else {
		fprintf(f,"%-15s%d\n","Start:",part->start);
		fprintf(f,"%-15s%d\n","Sectors:",part->size);
	}

	fclose(f);
}
