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

#include <esc/ipc/clientdevice.h>
#include <esc/proto/net.h>
#include <esc/proto/nic.h>
#include <esc/proto/socket.h>
#include <sys/common.h>

struct Empty {
	size_t size() const {
		return 0;
	}
} A_ALIGNED(2);

template<class T = Empty>
class Ethernet;
template<class T = Empty>
class IPv4;
class Socket;

enum {
	ETHER_HEAD_SIZE		= 6 + 6 + 2
};

struct PendingRequest {
	bool isRead() {
		return (mid & 0xFFFF) == MSG_FILE_READ || (mid & 0xFFFF) == MSG_SOCK_RECVFROM;
	}
	bool isWrite() {
		return (mid & 0xFFFF) == MSG_FILE_WRITE || (mid & 0xFFFF) == MSG_SOCK_SENDTO;
	}

	msgid_t mid;
	size_t count;
	union {
		struct {
			void *data;
			bool needsSrc;
		} read;
		struct {
			const void *data;
			size_t remaining;
			uint32_t seqNo;
		} write;
		struct {
			int fd;
			int devfd;
			esc::ClientDevice<Socket> *dev;
		} accept;
	} d;
};

static const uint16_t WELL_KNOWN_PORTS		= 0;
static const uint16_t REGISTERED_PORTS		= 1024;
static const uint16_t PRIVATE_PORTS			= 49152;
static const uint16_t PRIVATE_PORTS_CNT		= 65536 - PRIVATE_PORTS;
