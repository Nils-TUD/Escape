/**
 * $Id: disk.c 219 2011-06-08 07:35:56Z nasmussen $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "dev/disk.h"
#include "dev/timer.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "mmix/io.h"
#include "exception.h"
#include "config.h"
#include "sim.h"

#define DISK_CTRL		0					// control register
#define DISK_CNT		8					// sector count register
#define DISK_SCT		16					// disk sector register
#define DISK_CAP		24					// disk capacity register

#define DISK_STRT		0x01				// a 1 written here starts the disk command
#define DISK_IEN		0x02				// enable disk interrupt
#define DISK_WRT		0x04				// command type: 0 = read, 1 = write
#define DISK_ERR		0x08				// 0 = ok, 1 = error; valid when DONE = 1
#define DISK_DONE		0x10				// 1 = disk has finished the command
#define DISK_READY		0x20				// 1 = capacity valid, disk accepts command

static void disk_reset(void);
static void disk_shutdown(void);
static octa disk_read(octa addr,bool sideEffects);
static void disk_write(octa addr,octa data);
static octa disk_readOcta(byte *p);
static void disk_writeOcta(byte *p,octa data);
static void disk_callback(int n);

static sDevice diskDev;
static FILE *diskImage;
static long totalSectors;

static octa diskCtrl;
static octa diskCnt;
static octa diskSct;
static octa diskCap;

static byte diskBuffer[DISK_BUF_SIZE];

static long lastSct;

void disk_init(void) {
	const char *diskImageName = cfg_getDiskImage();
	if(diskImageName == NULL) {
		// do not install disk
		diskImage = NULL;
		totalSectors = 0;
	}
	else {
		// try to install disk
		diskImage = fopen(diskImageName,"r+b");
		if(diskImage == NULL)
			sim_error("cannot open disk image '%s'",diskImageName);
		fseek(diskImage,0,SEEK_END);
		long numBytes = ftell(diskImage);
		fseek(diskImage,0,SEEK_SET);
		if(numBytes % DISK_SECTOR_SIZE != 0)
			sim_error("disk image '%s' does not contain an integral number of sectors",diskImageName);
		totalSectors = numBytes / DISK_SECTOR_SIZE;

		// register device
		diskDev.name = "Disk";
		diskDev.startAddr = DISK_START_ADDR;
		diskDev.endAddr = DISK_START_ADDR + DISK_BUFFER + DISK_BUF_SIZE - 1;
		diskDev.irqMask = (octa)1 << DISK_IRQ;
		diskDev.reset = disk_reset;
		diskDev.read = disk_read;
		diskDev.write = disk_write;
		diskDev.shutdown = disk_shutdown;
		bus_register(&diskDev);
	}
	disk_reset();
}

static void disk_reset(void) {
	diskCtrl = 0;
	diskCnt = 0;
	diskSct = 0;
	diskCap = 0;
	lastSct = 0;
	if(totalSectors != 0)
		timer_start(DISK_STARTUP,&diskDev,disk_callback,0);
}

static void disk_shutdown(void) {
	if(diskImage != NULL)
		fclose(diskImage);
}

static octa disk_read(octa addr,bool sideEffects) {
	UNUSED(sideEffects);
	octa data = 0;
	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case DISK_CTRL:
			data = diskCtrl;
			break;

		case DISK_CNT:
			data = diskCnt;
			break;

		case DISK_SCT:
			data = diskSct;
			break;

		case DISK_CAP:
			data = diskCap;
			break;

		default:
			// buffer access
			if(reg >= DISK_BUFFER && reg < DISK_BUFFER + DISK_BUF_SIZE)
				data = disk_readOcta(diskBuffer + (reg & -(octa)sizeof(octa)) - DISK_BUFFER);
			// illegal register
			else
				ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
	return data;
}

static void disk_write(octa addr,octa data) {
	int reg = addr & DEV_REG_MASK;
	switch(reg) {
		case DISK_CTRL:
			if(data & DISK_WRT)
				diskCtrl |= DISK_WRT;
			else
				diskCtrl &= ~DISK_WRT;
			if(data & DISK_IEN)
				diskCtrl |= DISK_IEN;
			else
				diskCtrl &= ~DISK_IEN;

			if(data & DISK_STRT) {
				diskCtrl |= DISK_STRT;
				diskCtrl &= ~DISK_ERR;
				diskCtrl &= ~DISK_DONE;
				// only start a disk operation if disk is present
				if(diskCap != 0) {
					long delta = labs((long)diskSct - lastSct);
					if(delta > (long)diskCap)
						delta = diskCap;
					timer_start(DISK_DELAY + (delta * DISK_SEEK) / diskCap,&diskDev,disk_callback,1);
				}
			}
			else {
				diskCtrl &= ~DISK_STRT;
				if(data & DISK_ERR)
					diskCtrl |= DISK_ERR;
				else
					diskCtrl &= ~DISK_ERR;
				if(data & DISK_DONE)
					diskCtrl |= DISK_DONE;
				else
					diskCtrl &= ~DISK_DONE;
			}
			// raise/lower disk interrupt
			if((diskCtrl & DISK_IEN) != 0 && (diskCtrl & DISK_DONE) != 0)
				cpu_setInterrupt(DISK_IRQ);
			else
				cpu_resetInterrupt(DISK_IRQ);
			break;

		case DISK_CNT:
			diskCnt = data;
			break;

		case DISK_SCT:
			diskSct = data;
			break;

		case DISK_CAP:
			// this register is read-only
			ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;

		default:
			if(reg >= DISK_BUFFER && reg < DISK_BUFFER + DISK_BUF_SIZE)
				disk_writeOcta(diskBuffer + (reg & -(octa)sizeof(octa)) - DISK_BUFFER,data);
			// illegal register
			else
				ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
			break;
	}
}

static octa disk_readOcta(byte *p) {
	return ((octa)*(p + 0)) << 56 |
			((octa)*(p + 1)) << 48 |
			((octa)*(p + 2)) << 40 |
			((octa)*(p + 3)) << 32 |
			((octa)*(p + 4)) << 24 |
			((octa)*(p + 5)) << 16 |
			((octa)*(p + 6)) << 8 |
			((octa)*(p + 7)) << 0;
}

static void disk_writeOcta(byte *p,octa data) {
	*(p + 0) = (byte)(data >> 56);
	*(p + 1) = (byte)(data >> 48);
	*(p + 2) = (byte)(data >> 40);
	*(p + 3) = (byte)(data >> 32);
	*(p + 4) = (byte)(data >> 24);
	*(p + 5) = (byte)(data >> 16);
	*(p + 6) = (byte)(data >> 8);
	*(p + 7) = (byte)(data >> 0);
}

static void disk_callback(int n) {
	if(n == 0) {
		// startup time expired
		diskCap = totalSectors;
		diskCtrl |= DISK_READY;
		return;
	}

	// disk read or write
	size_t numScts = ((diskCnt - 1) & 0x07) + 1;
	if(diskCap != 0 && diskSct < diskCap && diskSct + numScts <= diskCap) {
		// do the transfer
		if(fseek(diskImage,diskSct * DISK_SECTOR_SIZE,SEEK_SET) != 0)
			sim_error("cannot position to sector in disk image");
		if(diskCtrl & DISK_WRT) {
			// buffer --> disk
			if(fwrite(diskBuffer,DISK_SECTOR_SIZE,numScts,diskImage) != numScts)
				sim_error("cannot write to disk image");
		}
		else {
			// disk --> buffer
			if(fread(diskBuffer,DISK_SECTOR_SIZE,numScts,diskImage) != numScts)
				sim_error("cannot read from disk image");
		}
		lastSct = (long)diskSct + (long)numScts - 1;
	}
	// sectors requested exceed disk capacity or we have no disk at all
	else
		diskCtrl |= DISK_ERR;

	diskCtrl &= ~DISK_STRT;
	diskCtrl |= DISK_DONE;
	// raise disk interrupt
	if(diskCtrl & DISK_IEN)
		cpu_setInterrupt(DISK_IRQ);
}
