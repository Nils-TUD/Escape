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

#include <esc/ipc/clientdevice.h>
#include <ecmxdisk/disk.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/irq.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/thread.h>
#include <usergroup/usergroup.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__eco32__)
#	define DISK_MODEL		"ECO32 Disk"
#else
#	define DISK_MODEL		"GIMMIX Disk"
#endif
#define MAX_RW_SIZE			(Disk::SECTOR_SIZE * 8)
#define IRQ_TIMEOUT			1000

using namespace esc;

static void createVFSEntry(const char *name,bool isPart);

static int irqSm;
static int drvId;
static ulong buffer[MAX_RW_SIZE / sizeof(ulong)];

class IRQDisk : public Disk {
public:
	explicit IRQDisk(ulong *regs,ulong *buf) : Disk(regs,buf,true) {
	}

	virtual bool wait() {
		volatile ulong *diskCtrlReg = _regs + REG_CTRL;
		while(!(*diskCtrlReg & (CTRL_DONE | CTRL_ERR))) {
			semdown(irqSm);
			if(!(*diskCtrlReg & (CTRL_DONE | CTRL_ERR)))
				printe("Waiting for interrupt: waked up with invalid status (%#x). Retrying.",*diskCtrlReg);
		}
		return (*diskCtrlReg & CTRL_ERR) == 0;
	}
};

static IRQDisk *disk;

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
		DevShFile::Request r(path,sizeof(path));
		is >> r;
		assert(c->shm() == NULL && !is.error());

		/* ensure that we don't cause pagefaults when accessing this memory. therefore,
		 * we populate it immediately and lock it into memory. additionally, we specify
		 * MAP_NOSWAP to let it fail if there is not enough memory instead of starting
		 * to swap (which would cause a deadlock, because we're doing that). */
		int res = joinshm(c,path,r.size,MAP_POPULATE | MAP_NOSWAP | MAP_LOCKED);
		is << DevShFile::Response(res) << Reply();
	}

	void read(IPCStream &is) {
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		size_t roffset = ROUND_DN(r.offset,Disk::SECTOR_SIZE);
		size_t rcount = ROUND_UP(r.count,Disk::SECTOR_SIZE);
		void *buf = r.shmemoff == -1 ? (void*)buffer : (*this)[is.fd()]->shm() + r.shmemoff;
		ssize_t res = 0;
		if(roffset + rcount <= disk->partCapacity() && roffset + rcount > roffset) {
			if(r.shmemoff != -1 || rcount <= MAX_RW_SIZE) {
				if(disk->read(buf,Disk::START_SECTOR + roffset / Disk::SECTOR_SIZE,rcount / Disk::SECTOR_SIZE))
					res = rcount;
			}
		}

		is << FileRead::Response::result(res) << Reply();
		if(r.shmemoff == -1 && res > 0)
			is << ReplyData(buf,res);
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;
		if(r.shmemoff == -1)
			is >> ReceiveData(buffer,sizeof(buffer));
		assert(!is.error());

		size_t roffset = ROUND_DN(r.offset,Disk::SECTOR_SIZE);
		size_t rcount = ROUND_UP(r.count,Disk::SECTOR_SIZE);
		void *buf = r.shmemoff == -1 ? (void*)buffer : (*this)[is.fd()]->shm() + r.shmemoff;
		ssize_t res = 0;
		if(roffset + rcount <= disk->partCapacity() && roffset + rcount > roffset) {
			if(r.shmemoff != -1 || rcount <= MAX_RW_SIZE) {
				if(disk->write(buf,Disk::START_SECTOR + roffset / Disk::SECTOR_SIZE,rcount / Disk::SECTOR_SIZE))
					res = rcount;
			}
		}

		is << FileWrite::Response::result(res) << Reply();
	}

	void size(IPCStream &is) {
		is << FileSize::Response::success(disk->partCapacity()) << Reply();
	}
};

int main(int argc,char **argv) {
	uintptr_t phys;

	if(argc < 2)
		error("Usage: %s <wait>",argv[0]);

	irqSm = semcrtirq(Disk::IRQ_NO,"Disk",NULL,NULL);
	if(irqSm < 0)
		error("Unable to create irq-semaphore");

	phys = Disk::BASE_ADDR;
	ulong *diskRegs = (ulong*)mmapphys(&phys,16,0,MAP_PHYS_MAP);
	if(diskRegs == NULL)
		error("Unable to map disk registers");
	phys = Disk::BUF_ADDR;
	ulong *diskBuf = (ulong*)mmapphys(&phys,MAX_RW_SIZE,0,MAP_PHYS_MAP);
	if(diskBuf == NULL)
		error("Unable to map disk buffer");

	/* check if disk is available and read the capacity */
	disk = new IRQDisk(diskRegs,diskBuf);
	if(!disk->present())
		error("Disk not found");

	print("Found disk with %lu sectors (%lu bytes)",
		disk->diskCapacity() / Disk::SECTOR_SIZE,disk->diskCapacity());

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
	delete disk;
	munmap(diskBuf);
	munmap(diskRegs);
	close(drvId);
	return EXIT_SUCCESS;
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
		fprintf(f,"%-15s%zu\n","Start:",Disk::START_SECTOR);
		fprintf(f,"%-15s%ld\n","Sectors:",(disk->diskCapacity() / Disk::SECTOR_SIZE) - Disk::START_SECTOR);
	}
	else {
		fprintf(f,"%-15s%s\n","Vendor:","THM");
		fprintf(f,"%-15s%s\n","Model:",DISK_MODEL);
		fprintf(f,"%-15s%ld\n","Sectors:",disk->diskCapacity() / Disk::SECTOR_SIZE);
	}
	fclose(f);
}
