/**
 * $Id: randmain.c 1091 2011-11-12 22:08:05Z nasmussen $
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
#include <esc/mem.h>
#include <usergroup/group.h>
#include <ipc/clientdevice.h>
#include <ipc/proto/disk.h>
#include <stdlib.h>
#include <assert.h>

using namespace ipc;

class RomDiskDevice : public ClientDevice<> {
public:
	explicit RomDiskDevice(const char *name,mode_t mode,size_t disksize,char *diskaddr)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE),
		  _disksize(disksize), _diskaddr(diskaddr) {
		set(MSG_FILE_READ,std::make_memfun(this,&RomDiskDevice::read));
		set(MSG_DISK_GETSIZE,std::make_memfun(this,&RomDiskDevice::size));
	}

	void read(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		ssize_t res = 0;
		if(r.offset + r.count <= _disksize && r.offset + r.count > r.offset)
			res = r.count;
		if(r.shmemoff != -1)
			memcpy(c->shm() + r.shmemoff,_diskaddr + r.offset,res);

		is << FileRead::Response(res) << Reply();
		if(r.shmemoff == -1 && res)
			is << ReplyData(_diskaddr + r.offset,res);
	}

	void size(IPCStream &is) {
		is << DiskSize::Response(_disksize) << Reply();
	}

private:
	size_t _disksize;
	char *_diskaddr;
};

int main(int argc,char **argv) {
	if(argc != 3)
		error("Usage: %s <wait> <module>",argv[0]);

	/* mmap the module */
	sFileInfo info;
	if(stat(argv[2],&info) < 0)
		error("Unable to stat '%s'",argv[2]);
	int fd = open(argv[2],IO_READ);
	if(fd < 0)
		error("Unable to open module '%s'",argv[2]);
	char *diskaddr = static_cast<char*>(mmap(NULL,info.size,info.size,PROT_READ,MAP_PRIVATE,fd,0));
	if(!diskaddr)
		error("Unable to map file '%s'",argv[2]);
	close(fd);

	RomDiskDevice romdisk("/dev/romdisk",0400,info.size,diskaddr);
	if(chown("/dev/romdisk",-1,GROUP_STORAGE) < 0)
		error("chown for /dev/romdisk failed");
	romdisk.loop();

	/* clean up */
	munmap(diskaddr);
	return EXIT_SUCCESS;
}
