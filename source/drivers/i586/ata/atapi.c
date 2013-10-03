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
#include <esc/proc.h>
#include "device.h"
#include "controller.h"
#include "ata.h"
#include "atapi.h"

static bool atapi_request(sATADevice *device,uint8_t *cmd,void *buffer,size_t bufSize);

void atapi_softReset(sATADevice *device) {
	int i = 1000000;
	ctrl_outb(device->ctrl,ATA_REG_DRIVE_SELECT,((device->id & SLAVE_BIT) << 4) | 0xA0);
	ctrl_wait(device->ctrl);
	ctrl_outb(device->ctrl,ATA_REG_COMMAND,COMMAND_ATAPI_RESET);
	while((ctrl_inb(device->ctrl,ATA_REG_STATUS) & CMD_ST_BUSY) && i--)
		ctrl_wait(device->ctrl);
	ctrl_outb(device->ctrl,ATA_REG_DRIVE_SELECT,(device->id << 4) | 0xA0);
	i = 1000000;
	while((ctrl_inb(device->ctrl,ATA_REG_CONTROL) & CMD_ST_BUSY) && i--)
		ctrl_wait(device->ctrl);
	ctrl_wait(device->ctrl);
}

bool atapi_read(sATADevice *device,uint op,void *buffer,uint64_t lba,A_UNUSED size_t secSize,
		size_t secCount) {
	uint8_t cmd[] = {SCSI_CMD_READ_SECTORS_EXT,0,0,0,0,0,0,0,0,0,0,0};
	if(!device->info.features.lba48)
		cmd[0] = SCSI_CMD_READ_SECTORS;
	/* no writing here ;) */
	if(op != OP_READ)
		return false;
	if(secCount == 0)
		return false;
	if(cmd[0] == SCSI_CMD_READ_SECTORS_EXT) {
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
	return atapi_request(device,cmd,buffer,secCount * device->secSize);
}

size_t atapi_getCapacity(sATADevice *device) {
	uint8_t resp[8];
	uint8_t cmd[] = {SCSI_CMD_READ_CAPACITY,0,0,0,0,0,0,0,0,0,0,0};
	bool res = atapi_request(device,cmd,resp,8);
	if(!res)
		return 0;
	return (resp[0] << 24) | (resp[1] << 16) | (resp[2] << 8) | (resp[3] << 0);
}

static bool atapi_request(sATADevice *device,uint8_t *cmd,void *buffer,size_t bufSize) {
	int res;
	size_t size;
	sATAController *ctrl = device->ctrl;

	/* send PACKET command to drive */
	if(!ata_readWrite(device,OP_PACKET,cmd,0xFFFF00,12,1))
		return false;

	/* now transfer the data */
	if(ctrl->useDma && device->info.capabilities.DMA)
		return ata_transferDMA(device,OP_READ,buffer,device->secSize,bufSize / device->secSize);

	/* ok, no DMA, so wait first until the drive is ready */
	res = ctrl_waitUntil(ctrl,ATAPI_TRANSFER_TIMEOUT,ATAPI_TRANSFER_SLEEPTIME,
			CMD_ST_DRQ,CMD_ST_BUSY);
	if(res == -1) {
		ATA_LOG("Device %d: Timeout before ATAPI-PIO-transfer",device->id);
		return false;
	}
	if(res != 0) {
		ATA_LOG("Device %d: ATAPI-PIO-transfer failed: %#x",device->id,res);
		return false;
	}

	/* read the actual size per transfer */
	ATA_PR2("Reading response-size");
	size = ((size_t)ctrl_inb(ctrl,ATA_REG_ADDRESS3) << 8) | (size_t)ctrl_inb(ctrl,ATA_REG_ADDRESS2);
	/* do the PIO-transfer (no check at the beginning; seems to cause trouble on some machines) */
	return ata_transferPIO(device,OP_READ,buffer,size,bufSize / size,false);
}
