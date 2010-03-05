/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/debug.h>
#include <esc/fileio.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/ports.h>
#include "drive.h"
#include "ata.h"
#include "atapi.h"

#define ATA_WAIT_TIMEOUT	500	/* ms */

static bool drive_isBusResponding(sATADrive* drive);
static bool drive_identify(sATADrive *drive,u8 cmd);

void drive_detect(sATADrive *drives,u32 count) {
	u32 i,s;
	u16 buffer[256];
	s = 0;
	for(i = 0; i < count; i++) {
		/* check for each bus if it's present */
		if(i % 2 == 0) {
			ATA_PR1("Checking if bus %d is present",i / 2);
			if(!drive_isBusResponding(drives + i)) {
				ATA_LOG("Bus %d not present",i / 2);
				i++;
				continue;
			}
		}

		ATA_PR1("Its present, sending 'IDENTIFY DEVICE' to device %d",i);
		/* first, identify the drive */
		if(!drive_identify(drives + i,COMMAND_IDENTIFY)) {
			ATA_PR1("Sending 'IDENTIFY PACKET DEVICE' to device %d",i);
			/* if that failed, try IDENTIFY PACKET DEVICE. Perhaps its an ATAPI-drive */
			if(!drive_identify(drives + i,COMMAND_IDENTIFY_PACKET)) {
				ATA_LOG("Device %d not present",i);
				continue;
			}
		}

		/* if it is present, read the partition-table */
		drives[i].present = 1;
		if(!drives[i].info.general.isATAPI) {
			drives[i].secSize = ATA_SEC_SIZE;
			drives[i].rwHandler = ata_readWrite;
			ATA_LOG("Device %d is an ATA-device",i);
			if(!ata_readWrite(drives + i,false,buffer,0,1)) {
				drives[i].present = 0;
				ATA_PR1("Drive %d: Unable to read partition-table!",i);
				continue;
			}

			/* copy partitions to mem */
			ATA_PR2("Parsing partition-table");
			part_fillPartitions(drives[i].partTable,buffer);
		}
		else {
			drives[i].secSize = ATAPI_SEC_SIZE;
			drives[i].rwHandler = atapi_read;
			/* pretend that the cd has 1 partition */
			drives[i].partTable[0].present = 1;
			drives[i].partTable[0].start = 0;
			drives[i].partTable[0].size = atapi_getCapacity(drives + i);
			ATA_LOG("Device %d is an ATAPI-device",i);
		}
	}
}

static bool drive_isBusResponding(sATADrive* drive) {
	s32 i;
	for(i = 1; i >= 0; i--) {
		/* begin with slave. master should respond if there is no slave */
		outByte(drive->basePort + REG_DRIVE_SELECT,i << 4);
		ata_wait(drive);

		/* write some arbitrary values to some registers */
		outByte(drive->basePort + REG_ADDRESS1,0xF1);
		outByte(drive->basePort + REG_ADDRESS2,0xF2);
		outByte(drive->basePort + REG_ADDRESS3,0xF3);

		/* if we can read them back, the bus is present */
		if(inByte(drive->basePort + REG_ADDRESS1) == 0xF1 &&
			inByte(drive->basePort + REG_ADDRESS2) == 0xF2 &&
			inByte(drive->basePort + REG_ADDRESS3) == 0xF3)
			return true;
	}
	return false;
}

static bool drive_identify(sATADrive *drive,u8 cmd) {
	u8 status;
	u16 *data;
	u32 x;
	u16 basePort = drive->basePort;

	ATA_PR2("Selecting device with port 0x%x",drive->basePort);
	outByte(drive->basePort + REG_DRIVE_SELECT,drive->slaveBit << 4);
	ata_wait(drive);

	/* disable interrupts */
	outByte(basePort + REG_CONTROL,CTRL_INTRPTS_ENABLED);

	/* check wether the drive exists */
	outByte(basePort + REG_COMMAND,cmd);
	status = inByte(basePort + REG_STATUS);
	ATA_PR1("Got 0x%x from status-port",status);
	if(status == 0)
		return false;
	else {
		u32 time = 0;
		ATA_PR2("Waiting for ATA-device");
		/* wait while busy; the other bits aren't valid while busy is set */
		while((inByte(basePort + REG_STATUS) & CMD_ST_BUSY) && time < ATA_WAIT_TIMEOUT) {
			time += 20;
			sleep(20);
		}
		/* wait a bit */
		ata_wait(drive);
		/* wait until ready (or error) */
		while(((status = inByte(basePort + REG_STATUS)) & (CMD_ST_BUSY | CMD_ST_DRQ)) != CMD_ST_DRQ &&
				time < ATA_WAIT_TIMEOUT) {
			if(status & CMD_ST_ERROR) {
				ATA_PR1("Error-bit in status set");
				return false;
			}
			time += 20;
			sleep(20);
		}
		if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) != CMD_ST_DRQ) {
			ATA_PR1("Timeout (%d ms)",ATA_WAIT_TIMEOUT);
			return false;
		}

		ATA_PR2("Reading information about device");
		/* drive ready */
		data = (u16*)&drive->info;
		for(x = 0; x < 256; x++)
			data[x] = inWord(basePort + REG_DATA);

		/* we don't support CHS atm */
		if(drive->info.capabilities.LBA == 0) {
			ATA_PR1("Device doesn't support LBA");
			return false;
		}

		return true;
	}
}

#if DEBUGGING

void drive_dbg_printInfo(sATADrive *drive) {
	u32 i;
	printf("oldCurCylinderCount = %u\n",drive->info.oldCurCylinderCount);
	printf("oldCurHeadCount = %u\n",drive->info.oldCurHeadCount);
	printf("oldCurSecsPerTrack = %u\n",drive->info.oldCurSecsPerTrack);
	printf("oldCylinderCount = %u\n",drive->info.oldCylinderCount);
	printf("oldHeadCount = %u\n",drive->info.oldHeadCount);
	printf("oldSecsPerTrack = %u\n",drive->info.oldSecsPerTrack);
	printf("oldswDMAActive = %u\n",drive->info.oldswDMAActive);
	printf("oldswDMASupported = %u\n",drive->info.oldswDMASupported);
	printf("oldUnformBytesPerSec = %u\n",drive->info.oldUnformBytesPerSec);
	printf("oldUnformBytesPerTrack = %u\n",drive->info.oldUnformBytesPerTrack);
	printf("curmaxSecsPerIntrpt = %u\n",drive->info.curmaxSecsPerIntrpt);
	printf("maxSecsPerIntrpt = %u\n",drive->info.maxSecsPerIntrpt);
	printf("firmwareRev = '");
	for(i = 0; i < 8; i += 2)
		printf("%c%c",drive->info.firmwareRev[i + 1],drive->info.firmwareRev[i]);
	printf("'\n");
	printf("modelNo = '");
	for(i = 0; i < 40; i += 2)
		printf("%c%c",drive->info.modelNo[i + 1],drive->info.modelNo[i]);
	printf("'\n");
	printf("serialNumber = '");
	for(i = 0; i < 20; i += 2)
		printf("%c%c",drive->info.serialNumber[i + 1],drive->info.serialNumber[i]);
	printf("'\n");
	printf("majorVer = 0x%02x\n",drive->info.majorVersion.raw);
	printf("minorVer = 0x%02x\n",drive->info.minorVersion);
	printf("general.isATAPI = %u\n",drive->info.general.isATAPI);
	printf("general.remMediaDevice = %u\n",drive->info.general.remMediaDevice);
	printf("mwDMAMode0Supp = %u\n",drive->info.mwDMAMode0Supp);
	printf("mwDMAMode0Sel = %u\n",drive->info.mwDMAMode0Sel);
	printf("mwDMAMode1Supp = %u\n",drive->info.mwDMAMode1Supp);
	printf("mwDMAMode1Sel = %u\n",drive->info.mwDMAMode1Sel);
	printf("mwDMAMode2Supp = %u\n",drive->info.mwDMAMode2Supp);
	printf("mwDMAMode2Sel = %u\n",drive->info.mwDMAMode2Sel);
	printf("minMwDMATransTimePerWord = %u\n",drive->info.minMwDMATransTimePerWord);
	printf("recMwDMATransTime = %u\n",drive->info.recMwDMATransTime);
	printf("minPIOTransTime = %u\n",drive->info.minPIOTransTime);
	printf("minPIOTransTimeIncCtrlFlow = %u\n",drive->info.minPIOTransTimeIncCtrlFlow);
	printf("multipleSecsValid = %u\n",drive->info.multipleSecsValid);
	printf("word88Valid = %u\n",drive->info.word88Valid);
	printf("words5458Valid = %u\n",drive->info.words5458Valid);
	printf("words6470Valid = %u\n",drive->info.words6470Valid);
	printf("userSectorCount = %u\n",drive->info.userSectorCount);
	printf("Capabilities:\n");
	printf("	DMA = %u\n",drive->info.capabilities.DMA);
	printf("	LBA = %u\n",drive->info.capabilities.LBA);
	printf("	IORDYDis = %u\n",drive->info.capabilities.IORDYDisabled);
	printf("	IORDYSup = %u\n",drive->info.capabilities.IORDYSupported);
	printf("Features:\n");
	printf("	APM = %u\n",drive->info.features.apm);
	printf("	autoAcousticMngmnt = %u\n",drive->info.features.autoAcousticMngmnt);
	printf("	CFA = %u\n",drive->info.features.cfa);
	printf("	devConfigOverlay = %u\n",drive->info.features.devConfigOverlay);
	printf("	deviceReset = %u\n",drive->info.features.deviceReset);
	printf("	downloadMicrocode = %u\n",drive->info.features.downloadMicrocode);
	printf("	flushCache = %u\n",drive->info.features.flushCache);
	printf("	flushCacheExt = %u\n",drive->info.features.flushCacheExt);
	printf("	hostProtArea = %u\n",drive->info.features.hostProtArea);
	printf("	lba48 = %u\n",drive->info.features.lba48);
	printf("	lookAhead = %u\n",drive->info.features.lookAhead);
	printf("	nop = %u\n",drive->info.features.nop);
	printf("	packet = %u\n",drive->info.features.packet);
	printf("	powerManagement = %u\n",drive->info.features.powerManagement);
	printf("	powerupStandby = %u\n",drive->info.features.powerupStandby);
	printf("	readBuffer = %u\n",drive->info.features.readBuffer);
	printf("	releaseInt = %u\n",drive->info.features.releaseInt);
	printf("	removableMedia = %u\n",drive->info.features.removableMedia);
	printf("	removableMediaSN = %u\n",drive->info.features.removableMediaSN);
	printf("	rwDMAQueued = %u\n",drive->info.features.rwDMAQueued);
	printf("	securityMode = %u\n",drive->info.features.securityMode);
	printf("	serviceInt = %u\n",drive->info.features.serviceInt);
	printf("	setFeaturesSpinup = %u\n",drive->info.features.setFeaturesSpinup);
	printf("	setMaxSecurity = %u\n",drive->info.features.setMaxSecurity);
	printf("	smart = %u\n",drive->info.features.smart);
	printf("	writeBuffer = %u\n",drive->info.features.writeBuffer);
	printf("	writeCache = %u\n",drive->info.features.writeCache);
	printf("\n");
}

#endif
