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

#ifndef LIST_H_
#define LIST_H_

#include <esc/common.h>
#include <esc/messages.h>

#define IOPORT_PCI_CFG_DATA			0xCFC
#define IOPORT_PCI_CFG_ADDR			0xCF8

/**
 * Detects PCI-devices
 */
void list_init(void);

/**
 * Finds a PCI-device by the class and subclass
 *
 * @param baseClass the class
 * @param subClass the subclass
 * @return the device or NULL
 */
sPCIDevice *list_getByClass(uchar baseClass,uchar subClass);

/**
 * Finds a PCI-device by bus, dev and func
 *
 * @param bus the bus
 * @param dev the device
 * @param func the function
 * @return the device or NULL
 */
sPCIDevice *list_getById(uchar bus,uchar dev,uchar func);

#endif /* LIST_H_ */
