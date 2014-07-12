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

#include <sys/common.h>

#include "../common.h"
#include "../proto/ethernet.h"
#include "../proto/ipv4.h"
#include "rawipsock.h"

RawSocketList RawIPSocket::sockets;

ssize_t RawIPSocket::sendto(msgid_t,const ipc::Socket::Addr *sa,const void *buffer,size_t size) {
	if(sa == NULL)
		return -EINVAL;

	const ipc::Net::IPv4Addr ip(sa->d.ipv4.addr);
	Route route = Route::find(ip);
	if(!route.valid())
		return -ENETUNREACH;

	const size_t total = Ethernet<>().size() + size;
	Ethernet<IPv4<>> *pkt = (Ethernet<IPv4<>>*)malloc(total);
	if(!pkt)
		return -ENOMEM;
	memcpy(&pkt->payload,buffer,size);
	Ethernet<> *epkt = reinterpret_cast<Ethernet<>*>(pkt);
	ssize_t res;
	if(route.flags & ipc::Net::FL_USE_GW)
		res = ARP::send(route.link,epkt,total,route.gateway,route.netmask,IPv4<>::ETHER_TYPE);
	else
		res = ARP::send(route.link,epkt,total,ip,route.netmask,IPv4<>::ETHER_TYPE);
	free(pkt);
	return res;
}
