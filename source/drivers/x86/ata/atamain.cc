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

#include <sys/common.h>
#include <usergroup/group.h>
#include <sys/arch/x86/ports.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/messages.h>
#include <sys/thread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <esc/ipc/clientdevice.h>
#include <esc/ipc/ipcstream.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vthrow.h>

#include "ata.h"
#include "controller.h"
#include "device.h"
#include "partition.h"

using namespace esc;

#define MAX_RW_SIZE		4096
#define RETRY_COUNT		3

class ATAPartitionDevice;

static ulong handleRead(sATADevice *device,sPartition *part,uint16_t *buf,uint offset,uint count);
static ulong handleWrite(sATADevice *device,sPartition *part,uint16_t *buf,uint offset,uint count);
static void initDrives(void);
static void createVFSEntry(sATADevice *device,sPartition *part,const char *name);

static size_t drvCount = 0;
static ATAPartitionDevice *devs[DEVICE_COUNT * PARTITION_COUNT];
/* don't use dynamic memory here since this may cause trouble with swapping (which we do) */
/* because if the heap hasn't enough memory and we request more when we should swap the kernel
 * may not have more memory and can't do anything about it */
static uint16_t buffer[MAX_RW_SIZE / sizeof(uint16_t)];

class ATAPartitionDevice : public ClientDevice<> {
public:
	explicit ATAPartitionDevice(uint dev,uint part,const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_SIZE | DEV_CLOSE),
		  _ataDev(ctrl_getDevice(dev)),
		  _part(_ataDev ? _ataDev->partTable + part : NULL) {
		set(MSG_DEV_SHFILE,std::make_memfun(this,&ATAPartitionDevice::shfile));
		set(MSG_FILE_READ,std::make_memfun(this,&ATAPartitionDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&ATAPartitionDevice::write));
		set(MSG_FILE_SIZE,std::make_memfun(this,&ATAPartitionDevice::size));

		if(_ataDev == NULL || _part == NULL || _ataDev->present == 0 || _part->present == 0)
			VTHROW("Invalid device/partition (dev=" << dev << ",part=" << part << ")");
	}

	void shfile(IPCStream &is) {
		char path[MAX_PATH_LEN];
		Client *c = (*this)[is.fd()];
		FileShFile::Request r(path,sizeof(path));
		is >> r;
		assert(c->shm() == NULL && !is.error());

		/* ensure that we don't cause pagefaults when accessing this memory. therefore,
		 * we populate it immediately and lock it into memory. additionally, we specify
		 * MAP_NOSWAP to let it fail if there is not enough memory instead of starting
		 * to swap (which would cause a deadlock, because we're doing that). */
		int res = joinshm(c,path,r.size,MAP_POPULATE | MAP_NOSWAP | MAP_LOCKED);
		is << FileShFile::Response(res) << Reply();
	}

	void read(IPCStream &is) {
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		uint16_t *buf = r.shmemoff == -1 ? buffer : (uint16_t*)(*this)[is.fd()]->shm() + (r.shmemoff >> 1);
		size_t res = handleRead(_ataDev,_part,buf,r.offset,r.count);

		is << FileRead::Response(res) << Reply();
		if(r.shmemoff == -1 && res > 0)
			is << ReplyData(buf,res);
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;
		if(r.shmemoff == -1)
			is >> ReceiveData(buffer,sizeof(buffer));
		assert(!is.error());

		uint16_t *buf = r.shmemoff == -1 ? buffer : (uint16_t*)(*this)[is.fd()]->shm() + (r.shmemoff >> 1);
		size_t res = handleWrite(_ataDev,_part,buf,r.offset,r.count);

		is << FileWrite::Response(res) << Reply();
	}

	void size(IPCStream &is) {
		is << FileSize::Response(_part->size * _ataDev->secSize) << Reply();
	}

private:
	sATADevice *_ataDev;
	sPartition *_part;
};

static int drive_thread(void *arg) {
	ATAPartitionDevice *dev = reinterpret_cast<ATAPartitionDevice*>(arg);
	dev->bindto(gettid());
	dev->loop();
	return 0;
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
	f = fopen("/sys/devices/ata","w");
	fclose(f);

	/* start drive threads */
	for(i = 1; i < drvCount; i++) {
		if(startthread(drive_thread, devs[i]) < 0)
			error("Unable to start thread");
	}

	/* mlock all regions to prevent that we're swapped out */
	if(mlockall() < 0)
		error("Unable to mlock regions");

	if(drvCount > 0)
		drive_thread(devs[0]);
	else
		print("No devices. Exiting");

	/* clean up */
	relports(ATA_REG_BASE_PRIMARY,8);
	relports(ATA_REG_BASE_SECONDARY,8);
	relport(ATA_REG_BASE_PRIMARY + ATA_REG_CONTROL);
	relport(ATA_REG_BASE_SECONDARY + ATA_REG_CONTROL);
	for(i = 0; i < drvCount; i++)
		delete devs[i];
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
	ATA_LOG("Invalid read-request: offset=%u, count=%u, partSize=%zu (device %d)",
			offset,count,part->size * ataDev->secSize,ataDev->id);
	return 0;
}

static ulong handleWrite(sATADevice *ataDev,sPartition *part,uint16_t *buf,uint offset,uint count) {
	if(offset + count <= part->size * ataDev->secSize && offset + count > offset) {
		if(buf != buffer || count <= MAX_RW_SIZE) {
			size_t i;
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
	ATA_LOG("Invalid write-request: offset=%u, count=%u, partSize=%zu (device %d)",
			offset,count,part->size * ataDev->secSize,ataDev->id);
	return 0;
}

static void initDrives(void) {
	uint deviceIds[] = {DEVICE_PRIM_MASTER,DEVICE_PRIM_SLAVE,DEVICE_SEC_MASTER,DEVICE_SEC_SLAVE};
	char name[SSTRLEN("hda1") + 1];
	char path[MAX_PATH_LEN] = "/dev/";
	for(size_t i = 0; i < DEVICE_COUNT; i++) {
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
		for(size_t p = 0; p < PARTITION_COUNT; p++) {
			if(ataDev->partTable[p].present) {
				if(!ataDev->info.general.isATAPI)
					snprintf(name,sizeof(name),"hd%c%zu",'a' + ataDev->id,p + 1);
				else
					snprintf(name,sizeof(name),"cd%c%zu",'a' + ataDev->id,p + 1);
				strcpy(path + SSTRLEN("/dev/"),name);

				try {
					devs[drvCount] = new ATAPartitionDevice(ataDev->id,p,path,0770);
					ATA_LOG("Registered device '%s' (device %d, partition %zu)",
							name,ataDev->id,p + 1);

					/* set group */
					strcpy(path + SSTRLEN("/dev/"),name);
					if(chown(path,-1,GROUP_STORAGE) < 0)
						ATA_LOG("Unable to set group for '%s'",path);
					createVFSEntry(ataDev,ataDev->partTable + p,name);
					drvCount++;
				}
				catch(const std::exception &) {
					ATA_LOG("Drive %d, Partition %zu: Unable to register device '%s'",
							ataDev->id,p + 1,name);
				}
			}
		}
	}
}

static void createVFSEntry(sATADevice *ataDev,sPartition *part,const char *name) {
	FILE *f;
	char path[SSTRLEN("/sys/devices/hda1") + 1];
	snprintf(path,sizeof(path),"/sys/devices/%s",name);

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
		fprintf(f,"%-15s%zu\n","Start:",part->start);
		fprintf(f,"%-15s%zu\n","Sectors:",part->size);
	}

	fclose(f);
}
