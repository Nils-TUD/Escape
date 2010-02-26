/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/service.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/signals.h>
#include <stdlib.h>
#include <errors.h>

#include "ata.h"
#include "drive.h"
#include "partition.h"

typedef struct {
	u32 drive;
	u32 partition;
} sDriver;

static sDriver *getDriver(tServ sid);
static void initDrives(void);
static void createVFSEntry(sATADrive *drive,sPartition *part,const char *name);

/* the drives */
static sATADrive drives[DRIVE_COUNT] = {
	/* primary master */
	{ .present = 0, .slaveBit = 0, .basePort = REG_BASE_PRIMARY },
	/* primary slave */
	{ .present = 0, .slaveBit = 1, .basePort = REG_BASE_PRIMARY },
	/* secondary master */
	{ .present = 0, .slaveBit = 0, .basePort = REG_BASE_SECONDARY },
	/* secondary slave */
	{ .present = 0, .slaveBit = 1, .basePort = REG_BASE_SECONDARY }
};
static u32 servCount = 0;
static tServ services[DRIVE_COUNT * PARTITION_COUNT];
static sDriver driver[DRIVE_COUNT * PARTITION_COUNT];

static sMsg msg;

int main(void) {
	u32 i;
	tServ client;
	tMsgId mid;

	/* request ports */
	/* for some reason virtualbox requires an additional port (9 instead of 8). Otherwise
	 * we are not able to access port (REG_BASE_PRIMARY + 7). */
	if(requestIOPorts(REG_BASE_PRIMARY,9) < 0 || requestIOPorts(REG_BASE_SECONDARY,9) < 0) {
		error("Unable to request ATA-port %d .. %d or %d .. %d",REG_BASE_PRIMARY,
				REG_BASE_PRIMARY + 7,REG_BASE_SECONDARY,REG_BASE_SECONDARY + 7);
	}
	if(requestIOPort(REG_BASE_PRIMARY + REG_CONTROL) < 0 ||
			requestIOPort(REG_BASE_SECONDARY + REG_CONTROL) < 0) {
		error("Unable to request ATA-port %d or %d",REG_BASE_PRIMARY + REG_CONTROL,
				REG_BASE_SECONDARY + REG_CONTROL);
	}

	/* detect and init all drives */
	ata_init();
	drive_detect(drives,DRIVE_COUNT);
	initDrives();
	/* flush prints */
	flush();

	/* we're ready now, so create a dummy-vfs-node that tells fs that all ata-drives are registered */
	tFile *f = fopen("/system/devices/ata","w");
	fclose(f);

	while(1) {
		tFD fd = getClient(services,servCount,&client);
		if(fd < 0) {
			if(fd != ERR_NO_CLIENT_WAITING)
				printe("[ata] Unable to get client");
			wait(EV_CLIENT);
		}
		else {
			sDriver *drv = getDriver(client);
			sATADrive *drive = drv == NULL ? NULL : (drives + drv->drive);
			sPartition *part = (drv == NULL || drive == NULL) ? NULL : (drive->partTable + drv->partition);
			if(drv == NULL || drive->present == 0 || part->present == 0) {
				/* should never happen */
				printe("[ata] Invalid drive");
				close(fd);
				continue;
			}

			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				ATA_PR2("Got message %d",mid);
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;

					case MSG_DRV_READ: {
						u16 *buffer = NULL;
						u32 offset = msg.args.arg1;
						u32 count = msg.args.arg2;
						msg.args.arg1 = 0;
						/* we have to check wether it is at least one sector. otherwise ATA can't
						 * handle the request */
						if(offset + count <= part->size * drive->secSize && offset + count > offset) {
							u32 rcount = (count + drive->secSize - 1) & ~(drive->secSize - 1);
							buffer = (u16*)malloc(rcount);
							if(buffer) {
								ATA_PR2("Reading %d bytes @ %x from drive 0x%x",rcount,offset,drive->basePort);
								if(drive->rwHandler(drive,false,buffer,
										offset / drive->secSize + part->start,rcount / drive->secSize)) {
									msg.data.arg1 = count;
								}
							}
						}
						msg.args.arg2 = true;
						send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
						if(buffer) {
							send(fd,MSG_DRV_READ_RESP,buffer,count);
							free(buffer);
						}
					}
					break;

					case MSG_DRV_WRITE: {
						u16 *buffer = NULL;
						u32 offset = msg.args.arg1;
						u32 count = msg.args.arg2;
						msg.args.arg1 = 0;
						if(offset + count <= part->size * drive->secSize && offset + count > offset) {
							buffer = (u16*)malloc(count);
							if(buffer) {
								if(receive(fd,&mid,buffer,count) >= 0) {
									ATA_PR2("Writing %d bytes @ %x to drive 0x%x",count,offset,drive->basePort);
									if(drive->rwHandler(drive,true,buffer,
											offset / drive->secSize + part->start,count / drive->secSize)) {
										msg.args.arg1 = count;
									}
								}
								free(buffer);
							}
						}
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_DRV_IOCTL: {
						msg.data.arg1 = ERR_UNSUPPORTED_OP;
						send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;

					case MSG_DRV_CLOSE:
						/* ignore */
						break;
				}
				ATA_PR2("Done");
			}

			close(fd);
		}
	}

	/* clean up */
	releaseIOPorts(REG_BASE_PRIMARY,8);
	releaseIOPorts(REG_BASE_SECONDARY,8);
	releaseIOPort(REG_BASE_PRIMARY + REG_CONTROL);
	releaseIOPort(REG_BASE_SECONDARY + REG_CONTROL);
	for(i = 0; i < servCount; i++)
		unregService(services[i]);
	return EXIT_SUCCESS;
}

static void initDrives(void) {
	char name[SSTRLEN("hda1") + 1];
	u32 i,p;
	for(i = 0; i < DRIVE_COUNT; i++) {
		if(drives[i].present == 0)
			continue;

		/* build VFS-entry */
		if(drives[i].info.general.isATAPI)
			snprintf(name,sizeof(name),"cd%c",'a' + i);
		else
			snprintf(name,sizeof(name),"hd%c",'a' + i);
		createVFSEntry(drives + i,NULL,name);

		/* register driver for every partition */
		for(p = 0; p < PARTITION_COUNT; p++) {
			if(drives[i].partTable[p].present) {
				if(!drives[i].info.general.isATAPI)
					snprintf(name,sizeof(name),"hd%c%d",'a' + i,p + 1);
				else
					snprintf(name,sizeof(name),"cd%c%d",'a' + i,p + 1);
				services[servCount] = regService(name,SERV_DRIVER);
				if(services[servCount] < 0)
					printf("Drive %d, Partition %d: Unable to register driver '%s'\n",i,p + 1,name);
				else {
					/* we're a block-device, so always data available */
					setDataReadable(services[servCount],true);
					createVFSEntry(drives + i,drives[i].partTable + p,name);
					driver[servCount].drive = i;
					driver[servCount].partition = p;
					servCount++;
				}
			}
		}
	}
}

static void createVFSEntry(sATADrive *drive,sPartition *part,const char *name) {
	tFile *f;
	char path[SSTRLEN("/system/devices/hda1") + 1];
	snprintf(path,sizeof(path),"/system/devices/%s",name);

	/*ATA_PR1("Creating '%s'",path);*/

	/* open and create file */
	f = fopen(path,"w");
	if(f == NULL) {
		printe("Unable to open '%s'",path);
		return;
	}

	if(part == NULL) {
		u32 i;
		fprintf(f,"%-15s%s\n","Type:",drive->info.general.isATAPI ? "ATAPI" : "ATA");
		fprintf(f,"%-15s","ModelNo:");
		for(i = 0; i < 40; i += 2)
			fprintf(f,"%c%c",drive->info.modelNo[i + 1],drive->info.modelNo[i]);
		fprintf(f,"\n");
		fprintf(f,"%-15s","SerialNo:");
		for(i = 0; i < 20; i += 2)
			fprintf(f,"%c%c",drive->info.serialNumber[i + 1],drive->info.serialNumber[i]);
		fprintf(f,"\n");
		if(drive->info.firmwareRev[0] && drive->info.firmwareRev[1]) {
			fprintf(f,"%-15s","FirmwareRev:");
			for(i = 0; i < 8; i += 2)
				fprintf(f,"%c%c",drive->info.firmwareRev[i + 1],drive->info.firmwareRev[i]);
			fprintf(f,"\n");
		}
		fprintf(f,"%-15s0x%02x\n","MajorVersion:",drive->info.majorVersion.raw);
		fprintf(f,"%-15s0x%02x\n","MinorVersion:",drive->info.minorVersion);
		fprintf(f,"%-15s%d\n","Sectors:",drive->info.userSectorCount);
		if(drive->info.words5458Valid) {
			fprintf(f,"%-15s%d\n","Cylinder:",drive->info.oldCylinderCount);
			fprintf(f,"%-15s%d\n","Heads:",drive->info.oldHeadCount);
			fprintf(f,"%-15s%d\n","SecsPerTrack:",drive->info.oldSecsPerTrack);
		}
		fprintf(f,"%-15s%d\n","LBA:",drive->info.capabilities.LBA);
		fprintf(f,"%-15s%d\n","LBA48:",drive->info.features.lba48);
		fprintf(f,"%-15s%d\n","DMA:",drive->info.capabilities.DMA);
	}
	else {
		fprintf(f,"%-15s%d\n","Start:",part->start);
		fprintf(f,"%-15s%d\n","Sectors:",part->size);
	}

	fclose(f);
}

static sDriver *getDriver(tServ sid) {
	u32 i;
	for(i = 0; i < servCount; i++) {
		if(services[i] == sid)
			return driver + i;
	}
	return NULL;
}
