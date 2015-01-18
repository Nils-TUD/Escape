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

#include <ecmxdisk/disk.h>
#include <sys/common.h>
#include <string.h>

#if DEBUG
#	define DISK_DBG(fmt,...)	print(fmt,## __VA_ARGS__);
#else
#	define DISK_DBG(...)
#endif

Disk::Disk(ulong *regs,ulong *buffer,bool irqs)
	: _ien(irqs ? CTRL_IEN : 0), _regs(regs), _buf(buffer),
	  _diskCap(getCapacity()), _partCap(_diskCap - START_SECTOR * SECTOR_SIZE) {
}

ulong Disk::getCapacity() {
	int i;
	volatile ulong *diskCtrlReg = _regs + REG_CTRL;
	ulong *diskCapReg = _regs + REG_CAP;
	/* wait for disk */
	for(i = 0; i < RDY_RETRIES; i++) {
		if(*diskCtrlReg & CTRL_READY)
			break;
	}
	if(i == RDY_RETRIES)
		return 0;
	*diskCtrlReg = CTRL_DONE;
	return *diskCapReg * SECTOR_SIZE;
}

bool Disk::read(void *buf,ulong secNo,ulong secCount) {
	ulong *diskSecReg = _regs + REG_SCT;
	ulong *diskCntReg = _regs + REG_CNT;
	ulong *diskCtrlReg = _regs + REG_CTRL;

	DISK_DBG("Reading sectors %d..%d ...",secNo,secNo + secCount - 1);

	/* maybe another request is active.. */
	if(!wait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* set sector and sector-count, start the disk-operation and wait */
	*diskSecReg = secNo;
	*diskCntReg = secCount;
	*diskCtrlReg = CTRL_STRT | _ien;

	if(!wait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* disk is ready, so copy from disk-buffer to memory */
	memcpy(buf,_buf,secCount * SECTOR_SIZE);
	DISK_DBG("done");
	return true;
}

bool Disk::write(const void *buf,ulong secNo,ulong secCount) {
	ulong *diskSecReg = _regs + REG_SCT;
	ulong *diskCntReg = _regs + REG_CNT;
	ulong *diskCtrlReg = _regs + REG_CTRL;

	DISK_DBG("Writing sectors %d..%d ...",secNo,secNo + secCount - 1);

	/* maybe another request is active.. */
	if(!wait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* disk is ready, so copy from memory to disk-buffer */
	memcpy(_buf,buf,secCount * SECTOR_SIZE);

	/* set sector and sector-count and start the disk-operation */
	*diskSecReg = secNo;
	*diskCntReg = secCount;
	*diskCtrlReg = CTRL_STRT | CTRL_WRT | _ien;
	/* we don't need to wait here because maybe there is no other request and we could therefore
	 * save time */
	DISK_DBG("done");
	return true;
}

bool Disk::wait() {
	volatile ulong *diskCtrlReg = _regs + REG_CTRL;
	while((*diskCtrlReg & (CTRL_DONE | CTRL_ERR)) == 0)
		;
	return (*diskCtrlReg & CTRL_ERR) == 0;
}
