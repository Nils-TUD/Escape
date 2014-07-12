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

#include <sys/common.h>
#include <ipc/proto/pci.h>

/**
 * Detects PCI-devices
 */
void list_init(void);

/**
 * Finds a PCI-device by the class and subclass
 *
 * @param baseClass the class
 * @param subClass the subclass
 * @param no the number (if there are multiple devices of this class/subclass)
 * @return the device or NULL
 */
ipc::PCI::Device *list_getByClass(uchar baseClass,uchar subClass,int no);

/**
 * Finds a PCI-device by bus, dev and func
 *
 * @param bus the bus
 * @param dev the device
 * @param func the function
 * @return the device or NULL
 */
ipc::PCI::Device *list_getById(uchar bus,uchar dev,uchar func);

/**
 * @param i the number
 * @return the PCI-device number <i>
 */
ipc::PCI::Device *list_get(size_t i);

/**
 * @return the number of devices
 */
size_t list_length(void);
