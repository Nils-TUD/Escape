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
#include <esc/ports.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "controller.h"
#include "ata.h"

/*static bool ata_transferPIO(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount);
static bool ata_transferDMA(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount);*/
static bool ata_setupCommand(sATADevice *device,u64 lba,u16 secCount,u8 cmd);
static u8 ata_getCommand(sATADevice *device,bool opWrite,u16 secSize);

bool ata_readWrite(sATADevice *device,bool opWrite,u16 *buffer,u64 lba,u16 secSize,u16 secCount) {
	u8 cmd = ata_getCommand(device,opWrite,secSize);
	if(!ata_setupCommand(device,lba,secCount,cmd))
		return false;

	switch(cmd) {
		case COMMAND_PACKET:
		case COMMAND_READ_SEC:
		case COMMAND_READ_SEC_EXT:
		case COMMAND_WRITE_SEC:
		case COMMAND_WRITE_SEC_EXT:
			return ata_transferPIO(device,opWrite,buffer,secSize,secCount,true);
		case COMMAND_READ_DMA:
		case COMMAND_READ_DMA_EXT:
		case COMMAND_WRITE_DMA:
		case COMMAND_WRITE_DMA_EXT:
			return ata_transferDMA(device,opWrite,buffer,secSize,secCount);
	}
	return false;
}

bool ata_transferPIO(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount,bool waitFirst) {
	s32 i;
	u16 *buf = buffer;
	sATAController *ctrl = device->ctrl;
	for(i = 0; i < secCount; i++) {
		u8 status;
		if(i > 0 || waitFirst) {
			do {
				if(opWrite) {
					ATA_PR2("Writing: wait until not busy");
					status = ctrl_inb(ctrl,ATA_REG_STATUS);
					while(status & CMD_ST_BUSY) {
						sleep(20);
						status = ctrl_inb(ctrl,ATA_REG_STATUS);
					}
				}
				else {
					ATA_PR2("Reading: wait for interrupt");
					ctrl_waitIntrpt(ctrl);
					status = ctrl_inb(ctrl,ATA_REG_STATUS);
				}

				if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
					break;
				if((status & CMD_ST_ERROR) != 0) {
					printf("[ata] error: %#x\n",ctrl_inb(ctrl,ATA_REG_ERROR));
					return false;
				}

	#if 0
				if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == 0)
					ctrl_softReset(ctrl);
	#endif
			}
			while(true);
		}

		/* now read / write the data */
		ATA_PR2("Ready, starting read/write");
		if(opWrite) {
			ctrl_outwords(ctrl,ATA_REG_DATA,buf,secSize / sizeof(u16));
			buf += secSize / sizeof(u16);
		}
		else {
			ctrl_inwords(ctrl,ATA_REG_DATA,buf,secSize / sizeof(u16));
			buf += secSize / sizeof(u16);
		}
		ATA_PR2("Transfer done");
	}
	ATA_PR2("All sectors done");
	return true;
}

bool ata_transferDMA(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount) {
	sATAController* ctrl = device->ctrl;
	u8 status;
	u32 size = secCount * secSize;

	/* setup PRDT */
	ctrl->dma_prdt_virt->buffer = ctrl->dma_buf_phys;
	ctrl->dma_prdt_virt->byteCount = size;
	ctrl->dma_prdt_virt->last = 1;

	/* stop running transfers */
	ATA_PR2("Stopping running transfers");
	ctrl_outbmrb(ctrl,BMR_REG_COMMAND,0);
	status = ctrl_inbmrb(ctrl,BMR_REG_STATUS) | BMR_STATUS_ERROR | BMR_STATUS_IRQ;
	ctrl_outbmrb(ctrl,BMR_REG_STATUS,status);

	/* set PRDT */
	ATA_PR2("Setting PRDT");
	ctrl_outbmrl(ctrl,BMR_REG_PRDT,(u32)ctrl->dma_prdt_phys);

	/* write data to buffer, if we should write */
	/* TODO we should use the buffer directly when reading from the client */
	if(opWrite)
		memcpy(ctrl->dma_buf_virt,buffer,size);

	/* it seems to be necessary to read those ports here */
	ATA_PR2("Starting DMA-transfer");
	ctrl_inbmrb(ctrl,BMR_REG_COMMAND);
	ctrl_inbmrb(ctrl,BMR_REG_STATUS);
    /* start bus-mastering */
	if(!opWrite)
		ctrl_outbmrb(ctrl,BMR_REG_COMMAND,BMR_CMD_START | BMR_CMD_READ);
	else
		ctrl_outbmrb(ctrl,BMR_REG_COMMAND,BMR_CMD_START);
	ctrl_inbmrb(ctrl,BMR_REG_COMMAND);
	ctrl_inbmrb(ctrl,BMR_REG_STATUS);

	/* now wait for an interrupt */
	ATA_PR2("Waiting for an interrupt");
	ctrl_waitIntrpt(ctrl);

	while(true) {
		status = ctrl_inb(ctrl,ATA_REG_STATUS);
		ATA_PR2("Checking status (%x)",status);
		if(status & CMD_ST_ERROR)
			return false;
		if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == 0) {
			ATA_PR2("Status OK");
			ctrl_inbmrb(ctrl,BMR_REG_STATUS);
			ctrl_outbmrb(ctrl,BMR_REG_COMMAND,0);
			/* copy data when reading */
			if(!opWrite)
				memcpy(buffer,ctrl->dma_buf_virt,size);
			return true;
		}
	}
	return false;
}

static bool ata_setupCommand(sATADevice *device,u64 lba,u16 secCount,u8 cmd) {
	sATAController *ctrl = device->ctrl;
	u8 devValue;

	if(secCount == 0)
		return false;

	if(!device->info.features.lba48) {
		if(lba & 0xFFFFFFFFF0000000LL) {
			printf("[ata] Trying to read from sector > 2^28-1\n");
			return false;
		}
		if(secCount & 0xFF00) {
			printf("[ata] Trying to read %u sectors with LBA28\n",secCount);
			return false;
		}

		/* For LBA28, the lowest 4 bits are bits 27-24 of LBA */
		devValue = DEVICE_LBA | ((device->id & SLAVE_BIT) << 4) | ((lba >> 24) & 0x0F);
		ctrl_outb(ctrl,ATA_REG_DRIVE_SELECT,devValue);
	}
	else {
		devValue = DEVICE_LBA | ((device->id & SLAVE_BIT) << 4);
		ctrl_outb(ctrl,ATA_REG_DRIVE_SELECT,devValue);
	}

	ATA_PR2("Selecting device %d (%s)",device->id,device->info.general.isATAPI ? "ATAPI" : "ATA");
	ctrl_wait(ctrl);

	ATA_PR2("Resetting control-register");
	ctrl_resetIrq(ctrl);
	/* reset control-register */
	ctrl_outb(ctrl,ATA_REG_CONTROL,0);

	/* needed for ATAPI */
	if(device->info.general.isATAPI)
		ctrl_outb(ctrl,ATA_REG_FEATURES,device->ctrl->useDma && device->info.capabilities.DMA);

	if(device->info.features.lba48) {
		ATA_PR2("LBA48: setting sector-count %d and LBA %x",secCount,(u32)(lba & 0xFFFFFFFF));
		/* LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 | */
		/*     48             32            16            0 */
		/* sector-count high-byte */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(u8)(secCount >> 8));
		/* LBA4, LBA5 and LBA6 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(u8)(lba >> 24));
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(u8)(lba >> 32));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(u8)(lba >> 40));
		/* sector-count low-byte */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(u8)(secCount & 0xFF));
		/* LBA1, LBA2 and LBA3 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(u8)(lba & 0xFF));
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(u8)(lba >> 8));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(u8)(lba >> 16));
	}
	else {
		ATA_PR2("LBA28: setting sector-count %d and LBA %x",secCount,(u32)(lba & 0xFFFFFFFF));
		/* send sector-count */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(u8)secCount);
		/* LBA1, LBA2 and LBA3 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(u8)lba);
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(u8)(lba >> 8));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(u8)(lba >> 16));
	}

	/* send command */
	ATA_PR2("Sending command %d",cmd);
	ctrl_outb(ctrl,ATA_REG_COMMAND,cmd);
	return true;
}

static u8 ata_getCommand(sATADevice *device,bool opWrite,u16 secSize) {
	static u8 commands[4][2] = {
		{COMMAND_READ_SEC,COMMAND_READ_SEC_EXT},
		{COMMAND_WRITE_SEC,COMMAND_WRITE_SEC_EXT},
		{COMMAND_READ_DMA,COMMAND_READ_DMA_EXT},
		{COMMAND_WRITE_DMA,COMMAND_WRITE_DMA_EXT}
	};
	u8 offset;
	if(device->secSize != secSize)
		return COMMAND_PACKET;
	offset = (device->ctrl->useDma && device->info.capabilities.DMA) ? 2 : 0;
	if(opWrite)
		offset++;
	if(device->info.features.lba48)
		return commands[offset][1];
	return commands[offset][0];
}
