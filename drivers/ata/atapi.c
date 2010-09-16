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
#include <esc/proc.h>
#include <esc/ports.h>
#include "device.h"
#include "controller.h"
#include "ata.h"
#include "atapi.h"

static bool atapi_request(sATADevice *device,u16 *cmd,u16 *buffer,u32 bufSize);

bool atapi_read(sATADevice *device,bool opWrite,u16 *buffer,u64 lba,u16 secSize,u16 secCount) {
	u8 cmd[] = {SCSI_CMD_READ_SECTORS_EXT,0,0,0,0,0,0,0,0,0,0,0};
	if(!device->info.features.lba48)
		cmd[0] = SCSI_CMD_READ_SECTORS;
	/* no writing here ;) */
	if(opWrite)
		return false;
	if(secCount == 0)
		return false;
	if(cmd == SCSI_CMD_READ_SECTORS_EXT) {
		cmd[6] = (secCount >> 24) & 0xFF;
		cmd[7] = (secCount >> 16) & 0xFF;
		cmd[8] = (secCount >> 8) & 0xFF;
		cmd[9] = (secCount >> 0) & 0xFF;
	}
	else {
		cmd[7] = (secCount >> 8) & 0xFF;
		cmd[8] = (secCount >> 0) & 0xFF;
	}
    cmd[2] = (lba >> 24) & 0xFF;
    cmd[3] = (lba >> 16) & 0xFF;
    cmd[4] = (lba >> 8) & 0xFF;
    cmd[5] = (lba >> 0) & 0xFF;
    return atapi_request(device,(u16*)cmd,buffer,secCount * device->secSize);
}

u32 atapi_getCapacity(sATADevice *device) {
	u8 resp[8];
	u8 cmd[] = {SCSI_CMD_READ_CAPACITY,0,0,0,0,0,0,0,0,0,0,0};
	bool res = atapi_request(device,(u16*)cmd,(u16*)resp,8);
	if(!res)
		return 0;
	return (resp[0] << 24) | (resp[1] << 16) | (resp[2] << 8) | (resp[3] << 0);
}

static bool atapi_request(sATADevice *device,u16 *cmd,u16 *buffer,u32 bufSize) {
	u8 status;
	u16 size;
	sATAController *ctrl = device->ctrl;

	/* send PACKET command to drive */
    if(!ata_readWrite(device,true,cmd,0xFFFF00,12,1))
    	return false;

    /* now transfer the data */
	if(ctrl->useDma && device->info.capabilities.DMA)
		return ata_transferDMA(device,false,buffer,device->secSize,bufSize / device->secSize);

	/* ok, no DMA, so wait first until the drive is ready */
	ATA_PR2("Waiting while busy");
	while(ctrl_inb(ctrl,ATA_REG_STATUS) & CMD_ST_BUSY)
		;
	ATA_PR2("Waiting until DRQ or ERROR set");
	while((!((status = ctrl_inb(ctrl,ATA_REG_STATUS)) & CMD_ST_DRQ)) && !(status & CMD_ST_ERROR))
		;

	/* read the actual size per transfer */
	ATA_PR2("Reading response-size");
	size = ((u16)ctrl_inb(ctrl,ATA_REG_ADDRESS3) << 8) | (u16)ctrl_inb(ctrl,ATA_REG_ADDRESS2);
	ATA_PR2("Got %u -> %u sectors",size,bufSize / size);
	/* do the PIO-transfer (no check at the beginning; seems to cause trouble on some machines) */
	return ata_transferPIO(device,false,buffer,size,bufSize / size,false);
}
