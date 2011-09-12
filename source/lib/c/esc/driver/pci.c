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
#include <esc/driver/pci.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <string.h>

static int pci_getDevice(sMsg *msg,msgid_t mid,sPCIDevice *dev);

int pci_getByClass(uchar base,uchar sub,sPCIDevice *d) {
	sMsg msg;
	msg.args.arg1 = base;
	msg.args.arg2 = sub;
	return pci_getDevice(&msg,MSG_PCI_GET_BY_CLASS,d);
}

int pci_getById(uchar bus,uchar dev,uchar func,sPCIDevice *d) {
	sMsg msg;
	msg.args.arg1 = bus;
	msg.args.arg2 = dev;
	msg.args.arg3 = func;
	return pci_getDevice(&msg,MSG_PCI_GET_BY_ID,d);
}

static int pci_getDevice(sMsg *msg,msgid_t mid,sPCIDevice *dev) {
	int res,fd;
	fd = open("/dev/pci",IO_MSGS);
	if(fd < 0)
		return fd;
	res = send(fd,mid,msg,sizeof(msg->args));
	if(res < 0) {
		close(fd);
		return res;
	}
	res = RETRY(receive(fd,NULL,msg,sizeof(msg->data)));
	if(res < 0) {
		close(fd);
		return res;
	}
	if(msg->data.arg1 == sizeof(sPCIDevice))
		memcpy(dev,msg->data.d,sizeof(sPCIDevice));
	close(fd);
	return msg->data.arg1;
}
