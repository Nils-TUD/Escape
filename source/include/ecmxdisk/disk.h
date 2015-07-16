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

#pragma once

#include <sys/common.h>

class Disk {
protected:
	enum {
		REG_CTRL	= 0,				/* control register */
		REG_CNT		= 1,				/* sector count register */
		REG_SCT		= 2,				/* disk sector register */
		REG_CAP		= 3,				/* disk capacity register */
	};

	enum {
		CTRL_STRT	= 0x01,				/* a 1 written here starts the disk command */
		CTRL_IEN	= 0x02,				/* enable disk interrupt */
		CTRL_WRT	= 0x04,				/* command type: 0 = read, 1 = write */
		CTRL_ERR	= 0x08,				/* 0 = ok, 1 = error; valid when DONE = 1 */
		CTRL_DONE	= 0x10,				/* 1 = disk has finished the command */
		CTRL_READY	= 0x20,				/* 1 = capacity valid, disk accepts command */
	};

	enum {
		RDY_RETRIES	= 10000000
	};

public:
#if defined(__eco32__)
	static const uintptr_t BASE_ADDR	= 0x30400000;
	static const uintptr_t BUF_ADDR		= 0x30480000;
	static const int IRQ_NO				= 0x8;
#else
	static const uintptr_t BASE_ADDR	= 0x0003000000000000;
	static const uintptr_t BUF_ADDR		= 0x0003000000080000;
	static const int IRQ_NO				= 0x33;
#endif

	static const size_t SECTOR_SIZE		= 512;
	static const size_t START_SECTOR	= 2048;		/* part 0 */

	explicit Disk(ulong *regs,ulong *buffer,bool irqs);
	virtual ~Disk() {
	}

	bool present() const {
		return _diskCap != 0;
	}
	ulong diskCapacity() const {
		return _diskCap;
	}
	ulong partCapacity() const {
		return _partCap;
	}

	bool read(void *buf,ulong secNo,ulong secCount);
	bool write(const void *buf,ulong secNo,ulong secCount);
	virtual bool wait();

protected:
	ulong getCapacity(void);

	ulong _ien;
	ulong *_regs;
	ulong *_buf;
	ulong _diskCap;
	ulong _partCap;
};
