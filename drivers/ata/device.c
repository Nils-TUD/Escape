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
#include <stdio.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/ports.h>
#include "device.h"
#include "controller.h"
#include "ata.h"
#include "atapi.h"

static bool device_identify(sATADevice *device,u8 cmd);

void device_init(sATADevice *device) {
	u16 buffer[256];
	ATA_PR1("Sending 'IDENTIFY DEVICE' to device %d",device->id);
	/* first, identify the device */
	if(!device_identify(device,COMMAND_IDENTIFY)) {
		ATA_PR1("Sending 'IDENTIFY PACKET DEVICE' to device %d",device->id);
		/* if that failed, try IDENTIFY PACKET DEVICE. Perhaps its an ATAPI-device */
		if(!device_identify(device,COMMAND_IDENTIFY_PACKET)) {
			ATA_LOG("Device %d not present",device->id);
			return;
		}
	}

	/* if it is present, read the partition-table */
	device->present = 1;
	if(!device->info.general.isATAPI) {
		device->secSize = ATA_SEC_SIZE;
		device->rwHandler = ata_readWrite;
		ATA_LOG("Device %d is an ATA-device",device->id);
		if(!ata_readWrite(device,OP_READ,buffer,0,device->secSize,1)) {
			device->present = 0;
			ATA_LOG("Device %d: Unable to read partition-table!",device->id);
			return;
		}

		/* copy partitions to mem */
		ATA_PR2("Parsing partition-table");
		part_fillPartitions(device->partTable,buffer);
	}
	else {
		device->secSize = ATAPI_SEC_SIZE;
		device->rwHandler = atapi_read;
		/* pretend that the cd has 1 partition */
		device->partTable[0].present = 1;
		device->partTable[0].start = 0;
		device->partTable[0].size = atapi_getCapacity(device);
		ATA_LOG("Device %d is an ATAPI-device",device->id);
	}

	if(device->ctrl->useDma && device->info.capabilities.DMA) {
		ATA_LOG("Device %d uses DMA",device->id);
	}
	else {
		ATA_LOG("Device %d uses PIO",device->id);
	}
}

static bool device_identify(sATADevice *device,u8 cmd) {
	u8 status;
	u16 *data;
	sATAController *ctrl = device->ctrl;

	ATA_PR2("Selecting device %d",device->id);
	ctrl_outb(ctrl,ATA_REG_DRIVE_SELECT,(device->id & SLAVE_BIT) << 4);
	ctrl_wait(ctrl);

	/* disable interrupts */
	ctrl_outb(ctrl,ATA_REG_CONTROL,CTRL_NIEN);

	/* check whether the device exists */
	ctrl_outb(ctrl,ATA_REG_COMMAND,cmd);
	status = ctrl_inb(ctrl,ATA_REG_STATUS);
	ATA_PR1("Got 0x%x from status-port",status);
	if(status == 0)
		return false;
	else {
		/* wait while busy; the other bits aren't valid while busy is set */
		s32 res = ctrl_waitUntil(ctrl,ATA_WAIT_TIMEOUT,ATA_WAIT_SLEEPTIME,0,CMD_ST_BUSY);
		if(res != 0)
			return false;
		/* wait a bit */
		ctrl_wait(ctrl);
		/* wait until ready (or error) */
		res = ctrl_waitUntil(ctrl,ATA_WAIT_TIMEOUT,ATA_WAIT_SLEEPTIME,CMD_ST_DRQ,CMD_ST_BUSY);
		if(res == -1) {
			ATA_LOG("Device %d: Timeout reached, assuming its not present",device->id);
			return false;
		}
		if(res != 0) {
			ATA_LOG("Device %d: Error %#x. Assuming its not present",device->id,res);
			return false;
		}

		ATA_PR2("Reading information about device");
		/* device ready */
		data = (u16*)&device->info;
		ctrl_inwords(ctrl,ATA_REG_DATA,data,256);

		/* we don't support CHS atm */
		if(device->info.capabilities.LBA == 0) {
			ATA_PR1("Device doesn't support LBA");
			return false;
		}

		return true;
	}
}

#if DEBUGGING

void device_dbg_printInfo(sATADevice *device) {
	u32 i;
	printf("oldCurCylinderCount = %u\n",device->info.oldCurCylinderCount);
	printf("oldCurHeadCount = %u\n",device->info.oldCurHeadCount);
	printf("oldCurSecsPerTrack = %u\n",device->info.oldCurSecsPerTrack);
	printf("oldCylinderCount = %u\n",device->info.oldCylinderCount);
	printf("oldHeadCount = %u\n",device->info.oldHeadCount);
	printf("oldSecsPerTrack = %u\n",device->info.oldSecsPerTrack);
	printf("oldswDMAActive = %u\n",device->info.oldswDMAActive);
	printf("oldswDMASupported = %u\n",device->info.oldswDMASupported);
	printf("oldUnformBytesPerSec = %u\n",device->info.oldUnformBytesPerSec);
	printf("oldUnformBytesPerTrack = %u\n",device->info.oldUnformBytesPerTrack);
	printf("curmaxSecsPerIntrpt = %u\n",device->info.curmaxSecsPerIntrpt);
	printf("maxSecsPerIntrpt = %u\n",device->info.maxSecsPerIntrpt);
	printf("firmwareRev = '");
	for(i = 0; i < 8; i += 2)
		printf("%c%c",device->info.firmwareRev[i + 1],device->info.firmwareRev[i]);
	printf("'\n");
	printf("modelNo = '");
	for(i = 0; i < 40; i += 2)
		printf("%c%c",device->info.modelNo[i + 1],device->info.modelNo[i]);
	printf("'\n");
	printf("serialNumber = '");
	for(i = 0; i < 20; i += 2)
		printf("%c%c",device->info.serialNumber[i + 1],device->info.serialNumber[i]);
	printf("'\n");
	printf("majorVer = 0x%02x\n",device->info.majorVersion.raw);
	printf("minorVer = 0x%02x\n",device->info.minorVersion);
	printf("general.isATAPI = %u\n",device->info.general.isATAPI);
	printf("general.remMediaDevice = %u\n",device->info.general.remMediaDevice);
	printf("mwDMAMode0Supp = %u\n",device->info.mwDMAMode0Supp);
	printf("mwDMAMode0Sel = %u\n",device->info.mwDMAMode0Sel);
	printf("mwDMAMode1Supp = %u\n",device->info.mwDMAMode1Supp);
	printf("mwDMAMode1Sel = %u\n",device->info.mwDMAMode1Sel);
	printf("mwDMAMode2Supp = %u\n",device->info.mwDMAMode2Supp);
	printf("mwDMAMode2Sel = %u\n",device->info.mwDMAMode2Sel);
	printf("minMwDMATransTimePerWord = %u\n",device->info.minMwDMATransTimePerWord);
	printf("recMwDMATransTime = %u\n",device->info.recMwDMATransTime);
	printf("minPIOTransTime = %u\n",device->info.minPIOTransTime);
	printf("minPIOTransTimeIncCtrlFlow = %u\n",device->info.minPIOTransTimeIncCtrlFlow);
	printf("multipleSecsValid = %u\n",device->info.multipleSecsValid);
	printf("word88Valid = %u\n",device->info.word88Valid);
	printf("words5458Valid = %u\n",device->info.words5458Valid);
	printf("words6470Valid = %u\n",device->info.words6470Valid);
	printf("userSectorCount = %u\n",device->info.userSectorCount);
	printf("Capabilities:\n");
	printf("	DMA = %u\n",device->info.capabilities.DMA);
	printf("	LBA = %u\n",device->info.capabilities.LBA);
	printf("	IORDYDis = %u\n",device->info.capabilities.IORDYDisabled);
	printf("	IORDYSup = %u\n",device->info.capabilities.IORDYSupported);
	printf("Features:\n");
	printf("	APM = %u\n",device->info.features.apm);
	printf("	autoAcousticMngmnt = %u\n",device->info.features.autoAcousticMngmnt);
	printf("	CFA = %u\n",device->info.features.cfa);
	printf("	devConfigOverlay = %u\n",device->info.features.devConfigOverlay);
	printf("	deviceReset = %u\n",device->info.features.deviceReset);
	printf("	downloadMicrocode = %u\n",device->info.features.downloadMicrocode);
	printf("	flushCache = %u\n",device->info.features.flushCache);
	printf("	flushCacheExt = %u\n",device->info.features.flushCacheExt);
	printf("	hostProtArea = %u\n",device->info.features.hostProtArea);
	printf("	lba48 = %u\n",device->info.features.lba48);
	printf("	lookAhead = %u\n",device->info.features.lookAhead);
	printf("	nop = %u\n",device->info.features.nop);
	printf("	packet = %u\n",device->info.features.packet);
	printf("	powerManagement = %u\n",device->info.features.powerManagement);
	printf("	powerupStandby = %u\n",device->info.features.powerupStandby);
	printf("	readBuffer = %u\n",device->info.features.readBuffer);
	printf("	releaseInt = %u\n",device->info.features.releaseInt);
	printf("	removableMedia = %u\n",device->info.features.removableMedia);
	printf("	removableMediaSN = %u\n",device->info.features.removableMediaSN);
	printf("	rwDMAQueued = %u\n",device->info.features.rwDMAQueued);
	printf("	securityMode = %u\n",device->info.features.securityMode);
	printf("	serviceInt = %u\n",device->info.features.serviceInt);
	printf("	setFeaturesSpinup = %u\n",device->info.features.setFeaturesSpinup);
	printf("	setMaxSecurity = %u\n",device->info.features.setMaxSecurity);
	printf("	smart = %u\n",device->info.features.smart);
	printf("	writeBuffer = %u\n",device->info.features.writeBuffer);
	printf("	writeCache = %u\n",device->info.features.writeCache);
	printf("\n");
}

#endif
