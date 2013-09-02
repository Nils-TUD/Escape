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
#include <esc/arch/i586/ports.h>
#include <esc/conf.h>
#include <esc/mem.h>
#include <stdio.h>
#include <assert.h>

#include "../../initerror.h"
#include "i586machine.h"

// source: NUL/NRE
void i586Machine::rebootPCI() {
	if(reqport(0xcf9) < 0) {
		printe("Unable to request port 0xcf9");
		return;
	}
	outbyte(0xcf9,(inbyte(0xcf9) & ~4) | 0x02);
    outbyte(0xcf9,0x06);
    outbyte(0xcf9,0x01);
}

// source: NUL/NRE
void i586Machine::rebootSysCtrlPort() {
	if(reqport(0x92) < 0) {
		printe("Unable to request port 0x92");
		return;
	}
    outbyte(0x92,0x01);
}

// source: NUL/NRE
void i586Machine::rebootACPI() {
	size_t len;
	FACP *facp = reinterpret_cast<FACP*>(mapTable("FACP",&len));
	if(facp == NULL || len < 129) {
		if(len < 129)
			fprintf(stderr,"FACP too small (%zu)\n",len);
		return;
	}

	if(~facp->flags & RESET_REG_SUP) {
		fprintf(stderr,"ACPI reset unsupported\n");
		return;
	}
	if(facp->RESET_REG.regBitWidth != 8) {
		fprintf(stderr,"Register width invalid (%u)\n",facp->RESET_REG.regBitWidth);
		return;
	}
	if(facp->RESET_REG.regBitOffset != 0) {
		fprintf(stderr,"Register offset invalid (%u)\n",facp->RESET_REG.regBitOffset);
		return;
	}
	if(facp->RESET_REG.accessSize > 1) {
		fprintf(stderr,"We need byte access\n");
		return;
	}

	uint8_t method = facp->RESET_REG.addressSpace;
	uint8_t value = facp->RESET_VALUE;
	uint64_t addr = facp->RESET_REG.address;
	fprintf(stderr,"Using method=%#x, value=%#x, addr=%#Lx\n",method,value,addr);

	switch(method) {
		case SYS_MEM: {
			uintptr_t phys = addr;
			volatile uint8_t *virt = reinterpret_cast<volatile uint8_t *>(regaddphys(&phys,1,0));
			if(!virt) {
				printe("Unable to map %p",addr);
				return;
			}
			*virt = value;
			break;
		}

		case SYS_IO: {
			if(reqport(addr) < 0) {
				printe("Unable to request port %#Lx",addr);
				return;
			}
			outbyte(addr,value);
			break;
		}

		case PCI_CONF_SPACE: {
			if(reqports(0xcf8,8) < 0) {
				printe("Unable to request ports %#x..%#x",0xcf8,0xcf8 + 7);
				return;
			}
            uint32_t val = (addr & 0x1f00000000ull) >> (32 - 11);
            val |= (addr & 0x70000) >> (16 - 8);
            val |= addr & 0x3c;
            outdword(0xcf8,val);
            outbyte(0xcf8 + (4 | (addr & 0x3)),val);
			break;
		}

		default:
			fprintf(stderr,"Unknown reset method %#x",method);
			break;
	}
}

void i586Machine::rebootPulseResetLine() {
	if(reqport(PORT_KB_DATA) < 0)
		throw init_error("Unable to request keyboard data-port");
	if(reqport(PORT_KB_CTRL) < 0)
		throw init_error("Unable to request keyboard-control-port");

	// wait until in-buffer empty
	while((inbyte(PORT_KB_CTRL) & 0x2) != 0)
		;
	// command 0xD1 to write the outputport
	outbyte(PORT_KB_CTRL,0xD1);
	// wait again until in-buffer empty
	while((inbyte(PORT_KB_CTRL) & 0x2) != 0)
		;
	// now set the new output-port for reset
	outbyte(PORT_KB_DATA,0xFE);
}

void i586Machine::reboot(Progress &pg) {
	pg.itemStarting("Trying reset via ACPI...");
	rebootACPI();

    pg.itemStarting("Trying reset via System Control Port A...\n");
    rebootSysCtrlPort();

	pg.itemStarting("Trying reset via PCI...");
	rebootPCI();

	pg.itemStarting("Trying reset via pulse-reset line of 8042 controller...");
	rebootPulseResetLine();
}

void *i586Machine::mapTable(const char *name,size_t *len) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/acpi/%s",name);
	int fd = open(path,IO_READ);
	if(fd < 0) {
		printe("open of %s failed",path);
		return NULL;
	}
	off_t off = seek(fd,0,SEEK_END);
	if(off < 0) {
		printe("seek in %s failed",path);
		return NULL;
	}
	*len = off;
	if(seek(fd,0,SEEK_SET) < 0) {
		printe("seek in %s failed",path);
		return NULL;
	}
	void *res = mmap(NULL,*len,*len,PROT_READ,MAP_PRIVATE,fd,0);
	if(res == NULL) {
		printe("mmap of %s failed",path);
		return NULL;
	}
	close(fd);
	return res;
}

// source: http://forum.osdev.org/viewtopic.php?t=16990
bool i586Machine::shutdownSupported(ShutdownInfo *info) {
	size_t len;
	// this is obviously not the intended way of using ACPI to power off the PC, because we would
	// need an AML interpreter for that, but it seems to work fine on quite a few machines.
	FACP *facp = reinterpret_cast<FACP*>(mapTable("FACP",&len));
	if(facp == NULL)
		return false;

	// map the DSDT
	sRSDT *dsdt = reinterpret_cast<sRSDT*>(mapTable("DSDT",&len));
	if(dsdt == NULL)
		return false;

	size_t dsdtLength = dsdt->length;
	uint8_t *ptr = reinterpret_cast<uint8_t*>(dsdt);

	// skip header
	ptr += 36 * 4;
	dsdtLength -= 36 * 4;
	while(dsdtLength-- > 0) {
		if(memcmp(ptr,"_S5_",4) == 0)
			break;
		ptr++;
	}
	if(dsdtLength <= 0) {
		fprintf(stderr,"_S5 not present\n");
		return false;
	}

	// check for valid AML structure
	if(!(*(ptr - 1) == 0x08 || (*(ptr - 2) == 0x08 && *(ptr - 1) == '\\')) || *(ptr + 4) != 0x12) {
		fprintf(stderr,"_S5 parse error\n");
		return false;
	}

	// calculate PkgLength size
	ptr += 5;
	ptr += ((*ptr & 0xC0) >> 6) + 2;

	// skip byteprefix
	if(*ptr == 0x0A)
		ptr++;
	info->SLP_TYPa = *(ptr) << 10;
	ptr++;

	// skip byteprefix
	if(*ptr == 0x0A)
		ptr++;
	info->SLP_TYPb = *(ptr) << 10;
	info->PM1a_CNT = facp->PM1a_CNT_BLK;
	info->PM1b_CNT = facp->PM1b_CNT_BLK;
	info->SLP_EN = 1 << 13;

	// request ports
	if(reqports(info->PM1a_CNT,2) < 0 || (info->PM1b_CNT != 0 && reqports(info->PM1b_CNT,2) < 0)) {
		fprintf(stderr,"Unable to request ports\n");
		return false;
	}
	return true;
}

void i586Machine::shutdown(Progress &pg) {
	ShutdownInfo info;
	if(shutdownSupported(&info)) {
		pg.itemStarting("Shutting down.");
		// send the shutdown command
		outword(info.PM1a_CNT,info.SLP_TYPa | info.SLP_EN);
		if(info.PM1b_CNT != 0)
			outword(info.PM1b_CNT,info.SLP_TYPb | info.SLP_EN);
	}
	else
		pg.itemStarting("You can turn off now.");
}
