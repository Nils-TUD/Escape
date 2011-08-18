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

.global ports_outByte
.global ports_outWord
.global ports_outDWord
.global ports_inByte
.global ports_inWord
.global ports_inDWord

# void ports_outByte(uint16_t port,uint8_t val);
ports_outByte:
	mov		4(%esp),%dx				# load port
	mov		8(%esp),%al				# load value
	out		%al,%dx					# write to port
	ret

# void ports_outWord(uint16_t port,uint16_t val);
ports_outWord:
	mov		4(%esp),%dx				# load port
	mov		8(%esp),%ax				# load value
	out		%ax,%dx					# write to port
	ret

# void ports_outWord(uint16_t port,uint32_t val);
ports_outDWord:
	mov		4(%esp),%dx				# load port
	mov		8(%esp),%eax			# load value
	out		%eax,%dx				# write to port
	ret

# uint8_t ports_inByte(uint16_t port);
ports_inByte:
	mov		4(%esp),%dx				# load port
	in		%dx,%al					# read from port
	ret

# uint16_t ports_inByte(uint16_t port);
ports_inWord:
	mov		4(%esp),%dx				# load port
	in		%dx,%ax					# read from port
	ret

# uint32_t ports_inByte(uint16_t port);
ports_inDWord:
	mov		4(%esp),%dx				# load port
	in		%dx,%eax				# read from port
	ret
