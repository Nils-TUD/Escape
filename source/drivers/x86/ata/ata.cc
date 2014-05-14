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

#include <esc/common.h>
#include <esc/arch/x86/ports.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "controller.h"
#include "ata.h"

static bool ata_setupCommand(sATADevice *device,uint64_t lba,size_t secCount,uint cmd);
static uint ata_getCommand(sATADevice *device,uint op);

bool ata_readWrite(sATADevice *device,uint op,void *buffer,uint64_t lba,size_t secSize,
		size_t secCount) {
	uint cmd = ata_getCommand(device,op);
	if(!ata_setupCommand(device,lba,secCount,cmd))
		return false;

	switch(cmd) {
		case COMMAND_PACKET:
		case COMMAND_READ_SEC:
		case COMMAND_READ_SEC_EXT:
		case COMMAND_WRITE_SEC:
		case COMMAND_WRITE_SEC_EXT:
			return ata_transferPIO(device,op,buffer,secSize,secCount,true);
		case COMMAND_READ_DMA:
		case COMMAND_READ_DMA_EXT:
		case COMMAND_WRITE_DMA:
		case COMMAND_WRITE_DMA_EXT:
			return ata_transferDMA(device,op,buffer,secSize,secCount);
	}
	return false;
}

bool ata_transferPIO(sATADevice *device,uint op,void *buffer,size_t secSize,
		size_t secCount,bool waitFirst) {
	size_t i;
	int res;
	uint16_t *buf = (uint16_t*)buffer;
	sATAController *ctrl = device->ctrl;
	for(i = 0; i < secCount; i++) {
		if(i > 0 || waitFirst) {
			if(op == OP_READ)
				ctrl_waitIntrpt(ctrl);
			res = ctrl_waitUntil(ctrl,PIO_TRANSFER_TIMEOUT,PIO_TRANSFER_SLEEPTIME,
					CMD_ST_READY,CMD_ST_BUSY);
			if(res == -1) {
				ATA_LOG("Device %d: Timeout before PIO-transfer",device->id);
				return false;
			}
			if(res != 0) {
				/* TODO ctrl_softReset(ctrl);*/
				ATA_LOG("Device %d: PIO-transfer failed: %#x",device->id,res);
				return false;
			}
		}

		/* now read / write the data */
		ATA_PR2("Ready, starting read/write");
		if(op == OP_READ)
			ctrl_inwords(ctrl,ATA_REG_DATA,buf,secSize / sizeof(uint16_t));
		else
			ctrl_outwords(ctrl,ATA_REG_DATA,buf,secSize / sizeof(uint16_t));
		buf += secSize / sizeof(uint16_t);
		ATA_PR2("Transfer done");
	}
	ATA_PR2("All sectors done");
	return true;
}

bool ata_transferDMA(sATADevice *device,uint op,void *buffer,size_t secSize,size_t secCount) {
	sATAController* ctrl = device->ctrl;
	uint8_t status;
	size_t size = secCount * secSize;
	int res;

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
	ctrl_outbmrl(ctrl,BMR_REG_PRDT,reinterpret_cast<uintptr_t>(ctrl->dma_prdt_phys));

	/* write data to buffer, if we should write */
	/* TODO we should use the buffer directly when reading from the client */
	if(op == OP_WRITE || op == OP_PACKET)
		memcpy(ctrl->dma_buf_virt,buffer,size);

	/* it seems to be necessary to read those ports here */
	ATA_PR2("Starting DMA-transfer");
	ctrl_inbmrb(ctrl,BMR_REG_COMMAND);
	ctrl_inbmrb(ctrl,BMR_REG_STATUS);
	/* start bus-mastering */
	if(op == OP_READ)
		ctrl_outbmrb(ctrl,BMR_REG_COMMAND,BMR_CMD_START | BMR_CMD_READ);
	else
		ctrl_outbmrb(ctrl,BMR_REG_COMMAND,BMR_CMD_START);
	ctrl_inbmrb(ctrl,BMR_REG_COMMAND);
	ctrl_inbmrb(ctrl,BMR_REG_STATUS);

	/* now wait for an interrupt */
	ATA_PR2("Waiting for an interrupt");
	ctrl_waitIntrpt(ctrl);

	res = ctrl_waitUntil(ctrl,DMA_TRANSFER_TIMEOUT,DMA_TRANSFER_SLEEPTIME,0,CMD_ST_BUSY | CMD_ST_DRQ);
	if(res == -1) {
		ATA_LOG("Device %d: Timeout after DMA-transfer",device->id);
		return false;
	}
	if(res != 0) {
		ATA_LOG("Device %d: DMA-Transfer failed: %#x",device->id,res);
		return false;
	}

	ctrl_inbmrb(ctrl,BMR_REG_STATUS);
	ctrl_outbmrb(ctrl,BMR_REG_COMMAND,0);
	/* copy data when reading */
	if(op == OP_READ)
		memcpy(buffer,ctrl->dma_buf_virt,size);
	return true;
}

static bool ata_setupCommand(sATADevice *device,uint64_t lba,size_t secCount,uint cmd) {
	sATAController *ctrl = device->ctrl;
	uint8_t devValue;

	if(secCount == 0)
		return false;

	if(!device->info.features.lba48) {
		if(lba & 0xFFFFFFFFF0000000LL) {
			ATA_LOG("Trying to read from %#Lx with LBA28",lba);
			return false;
		}
		if(secCount & 0xFF00) {
			ATA_LOG("Trying to read %zu sectors with LBA28",secCount);
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
	/* reset control-register */
	ctrl_outb(ctrl,ATA_REG_CONTROL,device->ctrl->useIrq ? 0 : CTRL_NIEN);

	/* needed for ATAPI */
	if(device->info.general.isATAPI)
		ctrl_outb(ctrl,ATA_REG_FEATURES,device->ctrl->useDma && device->info.capabilities.DMA);

	if(device->info.features.lba48) {
		ATA_PR2("LBA48: setting sector-count %d and LBA %x",secCount,(uint)(lba & 0xFFFFFFFF));
		/* LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 | */
		/*     48             32            16            0 */
		/* sector-count high-byte */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(uint8_t)(secCount >> 8));
		/* LBA4, LBA5 and LBA6 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(uint8_t)(lba >> 24));
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(uint8_t)(lba >> 32));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(uint8_t)(lba >> 40));
		/* sector-count low-byte */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(uint8_t)(secCount & 0xFF));
		/* LBA1, LBA2 and LBA3 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(uint8_t)(lba & 0xFF));
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(uint8_t)(lba >> 8));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(uint8_t)(lba >> 16));
	}
	else {
		ATA_PR2("LBA28: setting sector-count %d and LBA %x",secCount,(uint)(lba & 0xFFFFFFFF));
		/* send sector-count */
		ctrl_outb(ctrl,ATA_REG_SECTOR_COUNT,(uint8_t)secCount);
		/* LBA1, LBA2 and LBA3 */
		ctrl_outb(ctrl,ATA_REG_ADDRESS1,(uint8_t)lba);
		ctrl_outb(ctrl,ATA_REG_ADDRESS2,(uint8_t)(lba >> 8));
		ctrl_outb(ctrl,ATA_REG_ADDRESS3,(uint8_t)(lba >> 16));
	}

	/* send command */
	ATA_PR2("Sending command %d",cmd);
	ctrl_outb(ctrl,ATA_REG_COMMAND,cmd);
	return true;
}

static uint ata_getCommand(sATADevice *device,uint op) {
	static uint commands[4][2] = {
		{COMMAND_READ_SEC,COMMAND_READ_SEC_EXT},
		{COMMAND_WRITE_SEC,COMMAND_WRITE_SEC_EXT},
		{COMMAND_READ_DMA,COMMAND_READ_DMA_EXT},
		{COMMAND_WRITE_DMA,COMMAND_WRITE_DMA_EXT}
	};
	uint offset;
	if(op == OP_PACKET)
		return COMMAND_PACKET;
	offset = (device->ctrl->useDma && device->info.capabilities.DMA) ? 2 : 0;
	if(op == OP_WRITE)
		offset++;
	if(device->info.features.lba48)
		return commands[offset][1];
	return commands[offset][0];
}
