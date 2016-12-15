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

#include <esc/proto/net.h>
#include <sys/common.h>
#include <sys/endian.h>

namespace esc {

uint16_t Net::ipv4Checksum(const uint16_t *data,uint16_t length) {
	uint32_t sum = 0;
	for(; length > 1; length -= 2) {
		sum += *data++;
		if(sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);
	}
	if(length)
		sum += *data & 0xFF;

	while(sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	return ~sum;
}

uint16_t Net::ipv4PayloadChecksum(const Net::IPv4Addr &src,const Net::IPv4Addr &dst,uint16_t protocol,
		const uint16_t *header,size_t sz) {
	struct {
		alignas(sizeof(uint16_t)) esc::Net::IPv4Addr src;
		esc::Net::IPv4Addr dst;
		uint16_t proto;
		uint16_t dataSize;
	} A_PACKED pseudoHeader = {
		.src = src,
		.dst = dst,
		.proto = 0,
		.dataSize = static_cast<uint16_t>(cputobe16(sz))
	};
	pseudoHeader.proto = cputobe16(protocol);

	uint32_t checksum = 0;
	const uint16_t *data = reinterpret_cast<uint16_t*>(&pseudoHeader);
	for(size_t i = 0; i < sizeof(pseudoHeader) / 2; ++i)
		checksum += data[i];
	for(size_t i = 0; i < sz / 2; ++i)
		checksum += header[i];
	if((sz % 2) != 0)
		checksum += header[sz / 2] & 0xFF;

	while(checksum >> 16)
		checksum = (checksum & 0xFFFF) + (checksum >> 16);
	return ~checksum;
}

}
