/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/io.h>
#include <stdio.h>
#include <string.h>
#include <errors.h>

#define SECTOR_SIZE			512
#define START_SECTOR		32		/* part 0 */
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

#define DEBUG				0

#define DISK_LOG(fmt,...)	do { \
		printf("[DISK] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);

#if DEBUG
#define DISK_DBG(fmt,...)	do { \
		printf("[DISK] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);
#else
#define DISK_DBG(...)
#endif

static uint getDiskCapacity(void);
static bool diskRead(void *buf,uint secNo,uint secCount);
static bool diskWrite(const void *buf,uint secNo,uint secCount);
static bool diskWait(void);
static void regDrives(void);
static void createVFSEntry(const char *name,bool isPart);

static uint32_t *diskRegs;
static uint32_t *diskBuf;
static tFD drvId;
static uint diskCap = 0;
static uint partCap = 0;
static sMsg msg;
static uint32_t buffer[MAX_RW_SIZE / sizeof(uint32_t)];

int main(int argc,char **argv) {
	tMsgId mid;

	if(argc < 2) {
		printe("Usage: %s <wait>",argv[0]);
		return EXIT_FAILURE;
	}

	diskRegs = (uint32_t*)mapPhysical(DISK_BASE,16);
	if(diskRegs == NULL) {
		printe("Unable to map disk registers");
		return EXIT_FAILURE;
	}
	diskBuf = (uint32_t*)mapPhysical(DISK_BUF,MAX_RW_SIZE);
	if(diskBuf == NULL) {
		printe("Unable to map disk buffer");
		return EXIT_FAILURE;
	}

	/* check if disk is available and read the capacity */
	diskCap = getDiskCapacity();
	partCap = diskCap - START_SECTOR * SECTOR_SIZE;
	if(diskCap == 0) {
		printe("Disk not found");
		return EXIT_FAILURE;
	}

	DISK_LOG("Found disk with %u sectors (%u bytes)",diskCap / SECTOR_SIZE,diskCap);

	/* detect and init all devices */
	regDrives();
	/* flush prints */
	fflush(stdout);

	/* enable interrupts */
	uint *diskCtrl = diskRegs + DISK_CTRL;
	*diskCtrl = /*DISK_IEN | */DISK_DONE;

	/* we're ready now, so create a dummy-vfs-node that tells fs that the disk is registered */
	FILE *f = fopen("/system/devices/disk","w");
	fclose(f);

	while(1) {
		tFD fd = getWork(&drvId,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED)
				printe("[DISK] Unable to get client");
		}
		else {
#if 0
			switch(mid) {
				case MSG_DRV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					uint roffset = offset & ~(SECTOR_SIZE - 1);
					uint rcount = (count + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
					msg.args.arg1 = 0;
					if(roffset + rcount <= partCap && roffset + rcount > roffset) {
						if(rcount <= MAX_RW_SIZE) {
							if(diskRead(buffer,START_SECTOR + roffset / SECTOR_SIZE,
									rcount / SECTOR_SIZE)) {
								msg.data.arg1 = rcount;
							}
						}
					}
					msg.args.arg2 = true;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1 > 0)
						send(fd,MSG_DRV_READ_RESP,buffer,rcount);
				}
				break;

				case MSG_DRV_WRITE: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					uint roffset = offset & ~(SECTOR_SIZE - 1);
					uint rcount = (count + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
					msg.args.arg1 = 0;
					if(roffset + rcount <= partCap && roffset + rcount > roffset) {
						if(rcount <= MAX_RW_SIZE) {
							if(RETRY(receive(fd,&mid,buffer,rcount)) > 0) {
								if(diskWrite(buffer,START_SECTOR + roffset / SECTOR_SIZE,
										rcount / SECTOR_SIZE)) {
									msg.args.arg1 = rcount;
								}
							}
						}
					}
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;
			}
#else
			if(mid == MSG_DRV_READ) {
				uint offset = msg.args.arg1;
				uint count = msg.args.arg2;
				uint roffset = offset & ~(SECTOR_SIZE - 1);
				uint rcount = (count + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
				msg.args.arg1 = 0;
				if(roffset + rcount <= partCap && roffset + rcount > roffset) {
					if(rcount <= MAX_RW_SIZE) {
						if(diskRead(buffer,START_SECTOR + roffset / SECTOR_SIZE,
								rcount / SECTOR_SIZE)) {
							msg.data.arg1 = rcount;
						}
					}
				}
				msg.args.arg2 = true;
				send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
				if(msg.args.arg1 > 0)
					send(fd,MSG_DRV_READ_RESP,buffer,rcount);
			}
			else if(mid == MSG_DRV_WRITE) {
				uint offset = msg.args.arg1;
				uint count = msg.args.arg2;
				uint roffset = offset & ~(SECTOR_SIZE - 1);
				uint rcount = (count + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
				msg.args.arg1 = 0;
				if(roffset + rcount <= partCap && roffset + rcount > roffset) {
					if(rcount <= MAX_RW_SIZE) {
						if(RETRY(receive(fd,&mid,buffer,rcount)) > 0) {
							if(diskWrite(buffer,START_SECTOR + roffset / SECTOR_SIZE,
									rcount / SECTOR_SIZE)) {
								msg.args.arg1 = rcount;
							}
						}
					}
				}
				send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
			}
#endif
			close(fd);
		}
	}

	/* clean up */
	close(drvId);
	return EXIT_SUCCESS;
}

static uint getDiskCapacity(void) {
	int i;
	volatile uint *diskCtrlReg = diskRegs + DISK_CTRL;
	uint *diskCapReg = diskRegs + DISK_CAP;
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
	uint *diskSecReg = diskRegs + DISK_SCT;
	uint *diskCntReg = diskRegs + DISK_CNT;
	uint *diskCtrlReg = diskRegs + DISK_CTRL;

	DISK_DBG("Reading sectors %d..%d ...",secNo,secNo + secCount - 1);

	/* maybe another request is active.. */
	if(!diskWait()) {
		DISK_DBG("FAILED");
		return false;
	}

	/* set sector and sector-count, start the disk-operation and wait */
	*diskSecReg = secNo;
	*diskCntReg = secCount;
	*diskCtrlReg = DISK_STRT/* | DISK_IEN*/;

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
	uint *diskSecReg = diskRegs + DISK_SCT;
	uint *diskCntReg = diskRegs + DISK_CNT;
	uint *diskCtrlReg = diskRegs + DISK_CTRL;

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
	*diskCtrlReg = DISK_STRT | DISK_WRT/* | DISK_IEN*/;
	/* we don't need to wait here because maybe there is no other request and we could therefore
	 * save time */
	DISK_DBG("done");
	return true;
}

static bool diskWait(void) {
	/* TODO wait for interrupt */
	volatile uint *diskCtrlReg = diskRegs + DISK_CTRL;
	while(1) {
		if(*diskCtrlReg & (DISK_DONE | DISK_ERR))
			break;
	}
	return (*diskCtrlReg & DISK_ERR) == 0;
}

static void regDrives(void) {
	createVFSEntry("hda",false);
	drvId = regDriver("hda1",DRV_READ | DRV_WRITE);
	if(drvId < 0) {
		DISK_LOG("Drive 1, Partition 1: Unable to register driver 'hda1'");
	}
	else {
		DISK_LOG("Registered driver 'hda1' (device 1, partition 1)");
		/* we're a block-device, so always data available */
		fcntl(drvId,F_SETDATA,true);
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
