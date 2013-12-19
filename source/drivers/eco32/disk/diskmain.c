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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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

#define DISK_BASE			0x30400000
#define DISK_BUF			0x30480000
#define DISK_RDY_RETRIES	10000000

#define IRQ_TIMEOUT			1000

#define DEBUG				0

#define DISK_LOG(fmt,...)	do { \
		printf("[disk] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);

#if DEBUG
#define DISK_DBG(fmt,...)	do { \
		printf("[disk] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);
#else
#define DISK_DBG(...)
#endif

static void diskInterrupt(int sig);
static uint getDiskCapacity(void);
static bool diskRead(void *buf,uint secNo,uint secCount);
static bool diskWrite(const void *buf,uint secNo,uint secCount);
static bool diskWait(void);
static void regDrives(void);
static void createVFSEntry(const char *name,bool isPart);

static uint32_t *diskRegs;
static uint32_t *diskBuf;
static int drvId;
static uint diskCap = 0;
static uint partCap = 0;
static sMsg msg;
static uint32_t buffer[MAX_RW_SIZE / sizeof(uint32_t)];

int main(int argc,char **argv) {
	msgid_t mid;
	uintptr_t phys;

	if(argc < 2)
		error("Usage: %s <wait>",argv[0]);

	if(signal(SIG_INTRPT_ATA1,diskInterrupt) == SIG_ERR)
		error("Unable to announce disk-signal-handler");

	phys = DISK_BASE;
	diskRegs = (uint32_t*)regaddphys(&phys,16,0);
	if(diskRegs == NULL)
		error("Unable to map disk registers");
	phys = DISK_BUF;
	diskBuf = (uint32_t*)regaddphys(&phys,MAX_RW_SIZE,0);
	if(diskBuf == NULL)
		error("Unable to map disk buffer");

	/* check if disk is available and read the capacity */
	diskCap = getDiskCapacity();
	partCap = diskCap - START_SECTOR * SECTOR_SIZE;
	if(diskCap == 0)
		error("Disk not found");

	DISK_LOG("Found disk with %u sectors (%u bytes)",diskCap / SECTOR_SIZE,diskCap);

	/* detect and init all devices */
	regDrives();
	/* flush prints */
	fflush(stdout);

	/* enable interrupts */
	diskRegs[DISK_CTRL] = DISK_IEN | DISK_DONE;

	/* we're ready now, so create a dummy-vfs-node that tells fs that the disk is registered */
	FILE *f = fopen("/system/devices/disk","w");
	fclose(f);

	while(1) {
		int fd = getwork(drvId,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get client");
		}
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					uint roffset = ROUND_DN(offset,SECTOR_SIZE);
					uint rcount = ROUND_UP(count,SECTOR_SIZE);
					msg.args.arg1 = 0;
					if(roffset + rcount <= partCap && roffset + rcount > roffset) {
						if(rcount <= MAX_RW_SIZE) {
							if(diskRead(buffer,START_SECTOR + roffset / SECTOR_SIZE,
									rcount / SECTOR_SIZE)) {
								msg.data.arg1 = rcount;
							}
						}
					}
					msg.args.arg2 = READABLE_DONT_SET;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1 > 0)
						send(fd,MSG_DEV_READ_RESP,buffer,msg.data.arg1);
				}
				break;

				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					uint roffset = ROUND_DN(offset,SECTOR_SIZE);
					uint rcount = ROUND_UP(count,SECTOR_SIZE);
					msg.args.arg1 = 0;
					if(roffset + rcount <= partCap && roffset + rcount > roffset) {
						if(rcount <= MAX_RW_SIZE) {
							if(IGNSIGS(receive(fd,&mid,buffer,rcount)) > 0) {
								if(diskWrite(buffer,START_SECTOR + roffset / SECTOR_SIZE,
										rcount / SECTOR_SIZE)) {
									msg.args.arg1 = rcount;
								}
							}
						}
					}
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DISK_GETSIZE: {
					msg.args.arg1 = partCap;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE:
					close(fd);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	/* clean up */
	munmap(diskBuf);
	munmap(diskRegs);
	close(drvId);
	return EXIT_SUCCESS;
}

static void diskInterrupt(A_UNUSED int sig) {
	/* simply ignore the signal; most important is to interrupt the sleep call */
}

static uint getDiskCapacity(void) {
	int i;
	volatile uint32_t *diskCtrlReg = diskRegs + DISK_CTRL;
	uint32_t *diskCapReg = diskRegs + DISK_CAP;
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

static bool diskRead(void *buf,uint secNo,uint secCount) {
	uint32_t *diskSecReg = diskRegs + DISK_SCT;
	uint32_t *diskCntReg = diskRegs + DISK_CNT;
	uint32_t *diskCtrlReg = diskRegs + DISK_CTRL;

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

static bool diskWrite(const void *buf,uint secNo,uint secCount) {
	uint32_t *diskSecReg = diskRegs + DISK_SCT;
	uint32_t *diskCntReg = diskRegs + DISK_CNT;
	uint32_t *diskCtrlReg = diskRegs + DISK_CTRL;

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
	volatile uint32_t *diskCtrlReg = diskRegs + DISK_CTRL;
	if(!(*diskCtrlReg & (DISK_DONE | DISK_ERR))) {
		sleep(IRQ_TIMEOUT);
		if(!(*diskCtrlReg & (DISK_DONE | DISK_ERR))) {
			DISK_LOG("Waiting for interrupt: Timeout reached, giving up");
			return false;
		}
	}
	return (*diskCtrlReg & DISK_ERR) == 0;
}

static void regDrives(void) {
	createVFSEntry("hda",false);
	drvId = createdev("/dev/hda1",DEV_TYPE_BLOCK,DEV_READ | DEV_WRITE | DEV_CLOSE);
	if(drvId < 0) {
		DISK_LOG("Drive 1, Partition 1: Unable to register device 'hda1'");
	}
	else {
		DISK_LOG("Registered device 'hda1' (device 1, partition 1)");
		createVFSEntry("hda1",true);
	}
}

static void createVFSEntry(const char *name,bool isPart) {
	FILE *f;
	char path[SSTRLEN("/system/devices/hda1") + 1];
	snprintf(path,sizeof(path),"/system/devices/%s",name);

	/* open and create file */
	f = fopen(path,"w");
	if(f == NULL) {
		printe("Unable to open '%s'",path);
		return;
	}

	if(isPart) {
		fprintf(f,"%-15s%d\n","Start:",START_SECTOR);
		fprintf(f,"%-15s%d\n","Sectors:",(diskCap / SECTOR_SIZE) - START_SECTOR);
	}
	else {
		fprintf(f,"%-15s%s\n","Vendor:","THM");
		fprintf(f,"%-15s%s\n","Model:","ECO32 Disk");
		fprintf(f,"%-15s%d\n","Sectors:",diskCap / SECTOR_SIZE);
	}
	fclose(f);
}
