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
#include <sys/driver.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/thread.h>
#include <sys/io.h>
#include <sys/irq.h>
#include <usergroup/group.h>
#include <esc/ipc/clientdevice.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#if defined(__eco32__)
#	define DISK_BASE		0x30400000
#	define DISK_BUF			0x30480000
#	define DISK_IRQ			0x8
#	define DISK_MODEL		"ECO32 Disk"
#else
#	define DISK_BASE		0x0003000000000000
#	define DISK_BUF			0x0003000000080000
#	define DISK_IRQ			0x33
#	define DISK_MODEL		"GIMMIX Disk"
#endif

#define SECTOR_SIZE			512
#define START_SECTOR		128		/* part 0 */
#define MAX_RW_SIZE			(SECTOR_SIZE * 8)

#define DISK_CTRL			0		/* control register */
#define DISK_CNT			1		/* sector count register */
#define DISK_SCT			2		/* disk sector register */
#define DISK_CAP			3		/* disk capacity register */

#define DISK_STRT			0x01	/* a 1 written here starts the disk command */
#define DISK_IEN			0x02	/* enable disk interrupt */
#define DISK_WRT			0x04	/* command type: 0 = read, 1 = write */
#define DISK_ERR			0x08	/* 0 = ok, 1 = error; valid when DONE = 1 */
#define DISK_DONE			0x10	/* 1 = disk has finished the command */
#define DISK_READY			0x20	/* 1 = capacity valid, disk accepts command */

#define DISK_RDY_RETRIES	10000000

#define IRQ_TIMEOUT			1000

#define DEBUG				0

#if DEBUG
#	define DISK_DBG(fmt,...)	print(fmt,## __VA_ARGS__);
#else
#	define DISK_DBG(...)
#endif

using namespace esc;

static ulong getDiskCapacity(void);
static bool diskRead(void *buf,ulong secNo,ulong secCount);
static bool diskWrite(const void *buf,ulong secNo,ulong secCount);
static bool diskWait(void);
static void createVFSEntry(const char *name,bool isPart);

static int irqSm;
static ulong *diskRegs;
static ulong *diskBuf;
static int drvId;
static ulong diskCap = 0;
static ulong partCap = 0;
static ulong buffer[MAX_RW_SIZE / sizeof(ulong)];

class DiskDevice : public ClientDevice<> {
public:
	explicit DiskDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,
			DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_SIZE) {
		set(MSG_DEV_SHFILE,std::make_memfun(this,&DiskDevice::shfile));
		set(MSG_FILE_READ,std::make_memfun(this,&DiskDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&DiskDevice::write));
		set(MSG_FILE_SIZE,std::make_memfun(this,&DiskDevice::size));
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

		size_t roffset = ROUND_DN(r.offset,SECTOR_SIZE);
		size_t rcount = ROUND_UP(r.count,SECTOR_SIZE);
		void *buf = r.shmemoff == -1 ? (void*)buffer : (*this)[is.fd()]->shm() + r.shmemoff;
		ssize_t res = 0;
		if(roffset + rcount <= partCap && roffset + rcount > roffset) {
			if(r.shmemoff != -1 || rcount <= MAX_RW_SIZE) {
				if(diskRead(buf,START_SECTOR + roffset / SECTOR_SIZE,rcount / SECTOR_SIZE))
					res = rcount;
			}
		}

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

		size_t roffset = ROUND_DN(r.offset,SECTOR_SIZE);
		size_t rcount = ROUND_UP(r.count,SECTOR_SIZE);
		void *buf = r.shmemoff == -1 ? (void*)buffer : (*this)[is.fd()]->shm() + r.shmemoff;
		ssize_t res = 0;
		if(roffset + rcount <= partCap && roffset + rcount > roffset) {
			if(r.shmemoff != -1 || rcount <= MAX_RW_SIZE) {
				if(diskWrite(buf,START_SECTOR + roffset / SECTOR_SIZE,rcount / SECTOR_SIZE))
					res = rcount;
			}
		}

		is << FileWrite::Response(res) << Reply();
	}

	void size(IPCStream &is) {
		is << FileSize::Response(partCap) << Reply();
	}
};

int main(int argc,char **argv) {
	uintptr_t phys;

	if(argc < 2)
		error("Usage: %s <wait>",argv[0]);

	irqSm = semcrtirq(DISK_IRQ,"Disk",NULL,NULL);
	if(irqSm < 0)
		error("Unable to create irq-semaphore");

	phys = DISK_BASE;
	diskRegs = (ulong*)mmapphys(&phys,16,0,MAP_PHYS_MAP);
	if(diskRegs == NULL)
		error("Unable to map disk registers");
	phys = DISK_BUF;
	diskBuf = (ulong*)mmapphys(&phys,MAX_RW_SIZE,0,MAP_PHYS_MAP);
	if(diskBuf == NULL)
		error("Unable to map disk buffer");

	/* check if disk is available and read the capacity */
	diskCap = getDiskCapacity();
	partCap = diskCap - START_SECTOR * SECTOR_SIZE;
	if(diskCap == 0)
		error("Disk not found");

	print("Found disk with %lu sectors (%lu bytes)",diskCap / SECTOR_SIZE,diskCap);

	/* detect and init all devices */
	createVFSEntry("hda",false);
	DiskDevice dev("/dev/hda1",0770);
	if(chown("/dev/hda1",-1,GROUP_STORAGE) < 0)
		error("Unable to set group for '%s'","/dev/hda1");
	print("Registered device 'hda1' (device 1, partition 1)");
	createVFSEntry("hda1",true);
	/* flush prints */
	fflush(stdout);

	/* mlock all regions to prevent that we're swapped out */
	if(mlockall() < 0)
		error("Unable to mlock regions");

	/* we're ready now, so create a dummy-vfs-node that tells fs that the disk is registered */
	FILE *f = fopen("/sys/devices/disk","w");
	fclose(f);

	dev.loop();

	/* clean up */
	munmap(diskBuf);
	munmap(diskRegs);
	close(drvId);
	return EXIT_SUCCESS;
}

static ulong getDiskCapacity(void) {
	int i;
	volatile ulong *diskCtrlReg = diskRegs + DISK_CTRL;
	ulong *diskCapReg = diskRegs + DISK_CAP;
	/* wait for disk */
	for(i = 0; i < DISK_RDY_RETRIES; i++) {
		if(*diskCtrlReg & DISK_READY)
			break;
	}
	if(i == DISK_RDY_RETRIES)
		return 0;
	*diskCtrlReg = DISK_DONE;
	return *diskCapReg * SECTOR_SIZE;
}

static bool diskRead(void *buf,ulong secNo,ulong secCount) {
	ulong *diskSecReg = diskRegs + DISK_SCT;
	ulong *diskCntReg = diskRegs + DISK_CNT;
	ulong *diskCtrlReg = diskRegs + DISK_CTRL;

	DISK_DBG("Reading sectors %d..%d ...",secNo,secNo + secCount - 1);

	/* maybe another request is active.. */
	if(!diskWait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* set sector and sector-count, start the disk-operation and wait */
	*diskSecReg = secNo;
	*diskCntReg = secCount;
	*diskCtrlReg = DISK_STRT | DISK_IEN;

	if(!diskWait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* disk is ready, so copy from disk-buffer to memory */
	memcpy(buf,diskBuf,secCount * SECTOR_SIZE);
	DISK_DBG("done");
	return true;
}

static bool diskWrite(const void *buf,ulong secNo,ulong secCount) {
	ulong *diskSecReg = diskRegs + DISK_SCT;
	ulong *diskCntReg = diskRegs + DISK_CNT;
	ulong *diskCtrlReg = diskRegs + DISK_CTRL;

	DISK_DBG("Writing sectors %d..%d ...",secNo,secNo + secCount - 1);

	/* maybe another request is active.. */
	if(!diskWait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* disk is ready, so copy from memory to disk-buffer */
	memcpy(diskBuf,buf,secCount * SECTOR_SIZE);

	/* set sector and sector-count and start the disk-operation */
	*diskSecReg = secNo;
	*diskCntReg = secCount;
	*diskCtrlReg = DISK_STRT | DISK_WRT | DISK_IEN;
	/* we don't need to wait here because maybe there is no other request and we could therefore
	 * save time */
	DISK_DBG("done");
	return true;
}

static bool diskWait(void) {
	volatile ulong *diskCtrlReg = diskRegs + DISK_CTRL;
	while(!(*diskCtrlReg & (DISK_DONE | DISK_ERR))) {
		semdown(irqSm);
		if(!(*diskCtrlReg & (DISK_DONE | DISK_ERR)))
			printe("Waiting for interrupt: waked up with invalid status (%#x). Retrying.",*diskCtrlReg);
	}
	return (*diskCtrlReg & DISK_ERR) == 0;
}

static void createVFSEntry(const char *name,bool isPart) {
	FILE *f;
	char path[SSTRLEN("/sys/devices/hda1") + 1];
	snprintf(path,sizeof(path),"/sys/devices/%s",name);

	/* open and create file */
	f = fopen(path,"w");
	if(f == NULL) {
		printe("Unable to open '%s'",path);
		return;
	}

	if(isPart) {
		fprintf(f,"%-15s%d\n","Start:",START_SECTOR);
		fprintf(f,"%-15s%ld\n","Sectors:",(diskCap / SECTOR_SIZE) - START_SECTOR);
	}
	else {
		fprintf(f,"%-15s%s\n","Vendor:","THM");
		fprintf(f,"%-15s%s\n","Model:",DISK_MODEL);
		fprintf(f,"%-15s%ld\n","Sectors:",diskCap / SECTOR_SIZE);
	}
	fclose(f);
}
