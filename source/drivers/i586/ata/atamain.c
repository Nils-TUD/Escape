/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ata.h"
#include "controller.h"
#include "device.h"
#include "partition.h"

#define MAX_RW_SIZE		4096
#define RETRY_COUNT		3

typedef struct {
	uint device;
	uint partition;
} sId2Fd;

static ulong handleRead(sATADevice *device,sPartition *part,uint offset,uint count);
static ulong handleWrite(sATADevice *device,sPartition *part,int fd,uint offset,uint count);
static sId2Fd *getDevice(int sid);
static void initDrives(void);
static void createVFSEntry(sATADevice *device,sPartition *part,const char *name);

static size_t drvCount = 0;
static int devices[DEVICE_COUNT * PARTITION_COUNT];
static sId2Fd id2Fd[DEVICE_COUNT * PARTITION_COUNT];
/* don't use dynamic memory here since this may cause trouble with swapping (which we do) */
/* because if the heap hasn't enough memory and we request more when we should swap the kernel
 * may not have more memory and can't do anything about it */
static uint16_t buffer[MAX_RW_SIZE / sizeof(uint16_t)];

static sMsg msg;

int main(int argc,char **argv) {
	size_t i;
	msgid_t mid;
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

	while(1) {
		int drvFd;
		int fd = getWork(devices,drvCount,&drvFd,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("[ATA] Unable to get client");
		}
		else {
			sId2Fd *dev = getDevice(drvFd);
			sATADevice *ataDev = NULL;
			sPartition *part = NULL;
			if(dev) {
				ataDev = ctrl_getDevice(dev->device);
				if(ataDev)
					part = ataDev->partTable + dev->partition;
			}
			if(ataDev == NULL || part == NULL || ataDev->present == 0 || part->present == 0) {
				/* should never happen */
				printe("[ATA] Invalid device");
				continue;
			}

			ATA_PR2("Got message %d",mid);
			switch(mid) {
				case MSG_DEV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = handleRead(ataDev,part,offset,count);
					msg.args.arg2 = true;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1 > 0)
						send(fd,MSG_DEV_READ_RESP,buffer,count);
				}
				break;

				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = handleWrite(ataDev,part,fd,offset,count);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
			ATA_PR2("Done\n");
		}
	}

	/* clean up */
	releaseIOPorts(ATA_REG_BASE_PRIMARY,8);
	releaseIOPorts(ATA_REG_BASE_SECONDARY,8);
	releaseIOPort(ATA_REG_BASE_PRIMARY + ATA_REG_CONTROL);
	releaseIOPort(ATA_REG_BASE_SECONDARY + ATA_REG_CONTROL);
	for(i = 0; i < drvCount; i++)
		close(devices[i]);
	return EXIT_SUCCESS;
}

static ulong handleRead(sATADevice *ataDev,sPartition *part,uint offset,uint count) {
	/* we have to check whether it is at least one sector. otherwise ATA can't
	 * handle the request */
	if(offset + count <= part->size * ataDev->secSize && offset + count > offset) {
		uint rcount = (count + ataDev->secSize - 1) & ~(ataDev->secSize - 1);
		if(rcount <= MAX_RW_SIZE) {
			size_t i;
			ATA_PR2("Reading %d bytes @ %x from device %d",
					rcount,offset,ataDev->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Read failed; retry %zu",i);
				if(ataDev->rwHandler(ataDev,OP_READ,buffer,
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

static ulong handleWrite(sATADevice *ataDev,sPartition *part,int fd,uint offset,uint count) {
	msgid_t mid;
	if(offset + count <= part->size * ataDev->secSize && offset + count > offset) {
		if(count <= MAX_RW_SIZE) {
			size_t i;
			ssize_t res = RETRY(receive(fd,&mid,buffer,count));
			if(res <= 0)
				return res;

			ATA_PR2("Writing %d bytes @ %x to device %d",count,offset,ataDev->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Write failed; retry %zu",i);
				if(ataDev->rwHandler(ataDev,OP_WRITE,buffer,
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
				devices[drvCount] = createdev(path,DEV_TYPE_BLOCK,DEV_READ | DEV_WRITE);
				if(devices[drvCount] < 0) {
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
					id2Fd[drvCount].device = ataDev->id;
					id2Fd[drvCount].partition = p;
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

static sId2Fd *getDevice(int sid) {
	size_t i;
	for(i = 0; i < drvCount; i++) {
		if(devices[i] == sid)
			return id2Fd + i;
	}
	return NULL;
}
