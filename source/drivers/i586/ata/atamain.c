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
#include <errors.h>
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
static sId2Fd *getDriver(int sid);
static void initDrives(void);
static void createVFSEntry(sATADevice *device,sPartition *part,const char *name);

static size_t drvCount = 0;
static int drivers[DEVICE_COUNT * PARTITION_COUNT];
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
	FILE *f = fopen("/system/devices/ata","w");
	fclose(f);

	while(1) {
		int drvFd;
		int fd = getWork(drivers,drvCount,&drvFd,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED)
				printe("[ATA] Unable to get client");
		}
		else {
			sId2Fd *driver = getDriver(drvFd);
			sATADevice *device = NULL;
			sPartition *part = NULL;
			if(driver) {
				device = ctrl_getDevice(driver->device);
				if(device)
					part = device->partTable + driver->partition;
			}
			if(device == NULL || part == NULL || device->present == 0 || part->present == 0) {
				/* should never happen */
				printe("[ATA] Invalid device");
				continue;
			}

			ATA_PR2("Got message %d",mid);
			switch(mid) {
				case MSG_DRV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = handleRead(device,part,offset,count);
					msg.args.arg2 = true;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1 > 0)
						send(fd,MSG_DRV_READ_RESP,buffer,count);
				}
				break;

				case MSG_DRV_WRITE: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = handleWrite(device,part,fd,offset,count);
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
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
		close(drivers[i]);
	return EXIT_SUCCESS;
}

static ulong handleRead(sATADevice *device,sPartition *part,uint offset,uint count) {
	/* we have to check whether it is at least one sector. otherwise ATA can't
	 * handle the request */
	if(offset + count <= part->size * device->secSize && offset + count > offset) {
		uint rcount = (count + device->secSize - 1) & ~(device->secSize - 1);
		if(rcount <= MAX_RW_SIZE) {
			size_t i;
			ATA_PR2("Reading %d bytes @ %x from device %d",
					rcount,offset,device->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Read failed; retry %zu",i);
				if(device->rwHandler(device,OP_READ,buffer,
						offset / device->secSize + part->start,
						device->secSize,rcount / device->secSize)) {
					return count;
				}
			}
			ATA_LOG("Giving up after %zu retries",i);
			return 0;
		}
	}
	ATA_LOG("Invalid read-request: offset=%u, count=%u, partSize=%u (device %d)",
			offset,count,part->size * device->secSize,device->id);
	return 0;
}

static ulong handleWrite(sATADevice *device,sPartition *part,int fd,uint offset,uint count) {
	msgid_t mid;
	if(offset + count <= part->size * device->secSize && offset + count > offset) {
		if(count <= MAX_RW_SIZE) {
			size_t i;
			ssize_t res = RETRY(receive(fd,&mid,buffer,count));
			if(res <= 0)
				return res;

			ATA_PR2("Writing %d bytes @ %x to device %d",count,offset,device->id);
			for(i = 0; i < RETRY_COUNT; i++) {
				if(i > 0)
					ATA_LOG("Write failed; retry %zu",i);
				if(device->rwHandler(device,OP_WRITE,buffer,
						offset / device->secSize + part->start,
						device->secSize,count / device->secSize)) {
					return count;
				}
			}
			ATA_LOG("Giving up after %zu retries",i);
			return 0;
		}
	}
	ATA_LOG("Invalid write-request: offset=%u, count=%u, partSize=%u (device %d)",
			offset,count,part->size * device->secSize,device->id);
	return 0;
}

static void initDrives(void) {
	uint deviceIds[] = {DEVICE_PRIM_MASTER,DEVICE_PRIM_SLAVE,DEVICE_SEC_MASTER,DEVICE_SEC_SLAVE};
	char name[SSTRLEN("hda1") + 1];
	char path[MAX_PATH_LEN] = "/dev/";
	size_t i,p;
	for(i = 0; i < DEVICE_COUNT; i++) {
		sATADevice *device = ctrl_getDevice(deviceIds[i]);
		if(device->present == 0)
			continue;

		/* build VFS-entry */
		if(device->info.general.isATAPI)
			snprintf(name,sizeof(name),"cd%c",'a' + device->id);
		else
			snprintf(name,sizeof(name),"hd%c",'a' + device->id);
		createVFSEntry(device,NULL,name);

		/* register driver for every partition */
		for(p = 0; p < PARTITION_COUNT; p++) {
			if(device->partTable[p].present) {
				if(!device->info.general.isATAPI)
					snprintf(name,sizeof(name),"hd%c%d",'a' + device->id,p + 1);
				else
					snprintf(name,sizeof(name),"cd%c%d",'a' + device->id,p + 1);
				drivers[drvCount] = regDriver(name,DRV_READ | DRV_WRITE);
				if(drivers[drvCount] < 0) {
					ATA_LOG("Drive %d, Partition %d: Unable to register driver '%s'",
							device->id,p + 1,name);
				}
				else {
					ATA_LOG("Registered driver '%s' (device %d, partition %d)",
							name,device->id,p + 1);
					/* set group */
					strcpy(path + SSTRLEN("/dev/"),name);
					if(chown(path,-1,GROUP_STORAGE) < 0)
						ATA_LOG("Unable to set group for '%s'",path);
					/* we're a block-device, so always data available */
					fcntl(drivers[drvCount],F_SETDATA,true);
					createVFSEntry(device,device->partTable + p,name);
					id2Fd[drvCount].device = device->id;
					id2Fd[drvCount].partition = p;
					drvCount++;
				}
			}
		}
	}
}

static void createVFSEntry(sATADevice *device,sPartition *part,const char *name) {
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
		fprintf(f,"%-15s%s\n","Type:",device->info.general.isATAPI ? "ATAPI" : "ATA");
		fprintf(f,"%-15s","ModelNo:");
		for(i = 0; i < 40; i += 2)
			fprintf(f,"%c%c",device->info.modelNo[i + 1],device->info.modelNo[i]);
		fprintf(f,"\n");
		fprintf(f,"%-15s","SerialNo:");
		for(i = 0; i < 20; i += 2)
			fprintf(f,"%c%c",device->info.serialNumber[i + 1],device->info.serialNumber[i]);
		fprintf(f,"\n");
		if(device->info.firmwareRev[0] && device->info.firmwareRev[1]) {
			fprintf(f,"%-15s","FirmwareRev:");
			for(i = 0; i < 8; i += 2)
				fprintf(f,"%c%c",device->info.firmwareRev[i + 1],device->info.firmwareRev[i]);
			fprintf(f,"\n");
		}
		fprintf(f,"%-15s0x%02x\n","MajorVersion:",device->info.majorVersion.raw);
		fprintf(f,"%-15s0x%02x\n","MinorVersion:",device->info.minorVersion);
		fprintf(f,"%-15s%d\n","Sectors:",device->info.userSectorCount);
		if(device->info.words5458Valid) {
			fprintf(f,"%-15s%d\n","Cylinder:",device->info.oldCylinderCount);
			fprintf(f,"%-15s%d\n","Heads:",device->info.oldHeadCount);
			fprintf(f,"%-15s%d\n","SecsPerTrack:",device->info.oldSecsPerTrack);
		}
		fprintf(f,"%-15s%d\n","LBA:",device->info.capabilities.LBA);
		fprintf(f,"%-15s%d\n","LBA48:",device->info.features.lba48);
		fprintf(f,"%-15s%d\n","DMA:",device->info.capabilities.DMA);
	}
	else {
		fprintf(f,"%-15s%d\n","Start:",part->start);
		fprintf(f,"%-15s%d\n","Sectors:",part->size);
	}

	fclose(f);
}

static sId2Fd *getDriver(int sid) {
	size_t i;
	for(i = 0; i < drvCount; i++) {
		if(drivers[i] == sid)
			return id2Fd + i;
	}
	return NULL;
}
