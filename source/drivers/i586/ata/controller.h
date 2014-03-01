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

#pragma once

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

/**
 * Inits the controllers
 *
 * @param useDma whether to use DMA, if possible
 */
void ctrl_init(bool useDma);

/**
 * @param id the id
 * @return the ATA-device with given id
 */
sATADevice *ctrl_getDevice(uchar id);

/**
 * @param id the id
 * @return the ATA-Controller with given id
 */
sATAController *ctrl_getCtrl(uchar id);

/**
 * Writes <value> to the bus-master-register <reg> of the given controller
 *
 * @param ctrl the controller
 * @param reg the register
 * @param value the value
 */
void ctrl_outbmrb(sATAController *ctrl,uint16_t reg,uint8_t value);
void ctrl_outbmrl(sATAController *ctrl,uint16_t reg,uint32_t value);

/**
 * Reads a byte from the bus-master-register <reg> of the given controller
 *
 * @param ctrl the controller
 * @param reg the register
 * @return the value
 */
uint8_t ctrl_inbmrb(sATAController *ctrl,uint16_t reg);

/**
 * Writes <value> to the controller-register <reg>
 *
 * @param ctrl the controller
 * @param reg the register
 * @param value the value
 */
void ctrl_outb(sATAController *ctrl,uint16_t reg,uint8_t value);
/**
 * Writes <count> words from <buf> to the controller-register <reg>
 *
 * @param ctrl the controller
 * @param reg the register
 * @param buf the word-buffer
 * @param count the number of words
 */
void ctrl_outwords(sATAController *ctrl,uint16_t reg,const uint16_t *buf,size_t count);

/**
 * Reads a byte from the controller-register <reg>
 *
 * @param ctrl the controller
 * @param reg the register
 * @return the value
 */
uint8_t ctrl_inb(sATAController *ctrl,uint16_t reg);

/**
 * Reads <count> words from the controller-register <reg> into <buf>
 *
 * @param ctrl the controller
 * @param reg the register
 * @param buf the buffer to write the words to
 * @param count the number of words
 */
void ctrl_inwords(sATAController *ctrl,uint16_t reg,uint16_t *buf,size_t count);

/**
 * Performs a software-reset for the given controller
 *
 * @param ctrl the controller
 */
void ctrl_softReset(sATAController *ctrl);

/**
 * Waits for an interrupt with given controller
 *
 * @param ctrl the controller
 */
void ctrl_waitIntrpt(sATAController *ctrl);

/**
 * Waits for <set> to set and <unset> to unset in the status-register. If <sleepTime> is not zero,
 * it sleeps that number of milliseconds between the checks. Otherwise it checks it actively.
 * It gives up as soon as <timeout> is reached. That are milliseconds if sleepTime is not zero or
 * retries.
 *
 * @param ctrl the controller
 * @param timeout the timeout (milliseconds or retries)
 * @param sleepTime the number of milliseconds to sleep (0 = check actively)
 * @param set the bits to wait until they're set
 * @param unset the bits to wait until they're unset
 * @return 0 on success, -1 if timeout has been reached, other: value of the error-register
 */
int ctrl_waitUntil(sATAController *ctrl,time_t timeout,time_t sleepTime,uint8_t set,uint8_t unset);

/**
 * Performs a few io-port-reads (just to waste a bit of time ;))
 *
 * @param ctrl the controller
 */
void ctrl_wait(sATAController *ctrl);
