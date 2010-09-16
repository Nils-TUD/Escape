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

#ifndef CTRL_H_
#define CTRL_H_

#include <esc/common.h>
#include "device.h"

#define DEVICE_PRIMARY				0
#define DEVICE_SECONDARY			1

#define DEVICE_COUNT				4

/* device-identifier */
#define DEVICE_PRIM_MASTER			0
#define DEVICE_PRIM_SLAVE			1
#define DEVICE_SEC_MASTER			2
#define DEVICE_SEC_SLAVE			3

#define BMR_REG_COMMAND				0x0
#define BMR_REG_STATUS				0x2
#define BMR_REG_PRDT				0x4

#define BMR_STATUS_IRQ				0x4
#define BMR_STATUS_ERROR			0x2
#define BMR_STATUS_DMA				0x1

#define BMR_CMD_START				0x1
#define BMR_CMD_READ				0x8

void ctrl_init(void);

sATADevice *ctrl_getDevice(u8 id);
sATAController *ctrl_getCtrl(u8 id);

void ctrl_outbmrb(sATAController *ctrl,u16 reg,u8 value);
void ctrl_outbmrl(sATAController *ctrl,u16 reg,u32 value);

u8 ctrl_inbmrb(sATAController *ctrl,u16 reg);

void ctrl_outb(sATAController *ctrl,u16 reg,u8 value);
void ctrl_outwords(sATAController *ctrl,u16 reg,const u16 *buf,u32 count);

u8 ctrl_inb(sATAController *ctrl,u16 reg);
void ctrl_inwords(sATAController *ctrl,u16 reg,u16 *buf,u32 count);

void ctrl_softReset(sATAController *ctrl);

void ctrl_resetIrq(sATAController *ctrl);
void ctrl_waitIntrpt(sATAController *ctrl);
void ctrl_wait(sATAController *ctrl);

#endif /* CTRL_H_ */
