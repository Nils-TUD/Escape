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

#include <esc/ipc/clientdevice.h>
#include <sys/common.h>
#include <sys/mman.h>
#include <usergroup/usergroup.h>
#include <assert.h>
#include <stdlib.h>

using namespace esc;

class RamDiskDevice : public ClientDevice<> {
public:
	explicit RamDiskDevice(const char *name,mode_t mode,size_t disksize,char *diskaddr)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_DELEGATE | DEV_READ | DEV_WRITE | DEV_SIZE | DEV_CLOSE),
		  _disksize(disksize), _diskaddr(diskaddr) {
		set(MSG_FILE_READ,std::make_memfun(this,&RamDiskDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&RamDiskDevice::write));
		set(MSG_FILE_SIZE,std::make_memfun(this,&RamDiskDevice::size));
	}

	void read(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		ssize_t res = getCount(r.offset,r.count);
		if(r.shmemoff != -1)
			memcpy(c->shm() + r.shmemoff,_diskaddr + r.offset,res);

		is << FileRead::Response::result(res) << Reply();
		if(r.shmemoff == -1 && res)
			is << ReplyData(_diskaddr + r.offset,res);
	}

	void write(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileWrite::Request r;
		is >> r;

		ssize_t res = getCount(r.offset,r.count);
		if(r.shmemoff != -1)
			memcpy(_diskaddr + r.offset,c->shm() + r.shmemoff,res);
		else
			is >> ReceiveData(_diskaddr + r.offset,res);

		is << FileWrite::Response::result(res) << Reply();
	}

	void size(IPCStream &is) {
		is << FileSize::Response::success(_disksize) << Reply();
	}

private:
	size_t getCount(size_t offset,size_t count) {
		if(offset + count > offset) {
			if(offset + count > _disksize)
				return _disksize - offset;
			return count;
		}
		return 0;
	}

	size_t _disksize;
	char *_diskaddr;
};

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <device> -f <file>\n",name);
	fprintf(stderr,"Usage: %s <device> -m <size>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(argc != 4)
		usage(argv[0]);

	/* determine params */
	int fd = -1;
	char *device = argv[1];
	char *image = NULL;
	size_t size = 0;
	if(strcmp(argv[2],"-f") == 0) {
		image = argv[3];
		if(!isblock(image))
			error("'%s' is neither a block-device nor a regular file",image);

		fd = open(image,O_RDONLY);
		if(fd < 0)
			error("Unable to open module '%s'",image);
		size = filesize(fd);
	}
	else if(strcmp(argv[2],"-m") == 0)
		size = strtoul(argv[3],NULL,0);
	else
		usage(argv[0]);

	/* mmap file or anonymous memory */
	char *diskaddr = static_cast<char*>(
		mmap(NULL,size,argc > 2 ? size : 0,PROT_READ | PROT_WRITE,MAP_PRIVATE,fd,0));
	if(!diskaddr)
		error("mmap failed");
	if(fd != -1)
		close(fd);

	/* handle device */
	RamDiskDevice ramdisk(device,0600,size,diskaddr);
	if(chown(device,-1,GROUP_STORAGE) < 0)
		error("chown for '%s' failed",device);
	ramdisk.loop();

	/* clean up */
	munmap(diskaddr);
	return EXIT_SUCCESS;
}
