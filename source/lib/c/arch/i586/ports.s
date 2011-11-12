#
# $Id$
# Copyright (C) 2008 - 2009 Nils Asmussen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

.section .text

.global inbyte
.global inword
.global indword
.global outbyte
.global outword
.global outdword

# uint8_t inbyte(uint16_t port)
.type inbyte, @function
inbyte:
	mov		4(%esp),%dx							# load port
	in		%dx,%al								# read from port
	ret

# uint16_t inword(uint16_t port)
.type inword, @function
inword:
	mov		4(%esp),%dx							# load port
	in		%dx,%ax								# read from port
	ret

# uint32_t indword(uint16_t port)
.type indword, @function
indword:
	mov		4(%esp),%dx							# load port
	in		%dx,%eax							# read from port
	ret

# void outbyte(uint16_t port,uint8_t val)
.type outbyte, @function
outbyte:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%al							# load value
	out		%al,%dx								# write to port
	ret

# void outword(uint16_t port,uint16_t val)
.type outword, @function
outword:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%ax							# load value
	out		%ax,%dx								# write to port
	ret

# void outdword(uint16_t port,uint32_t val)
.type outdword, @function
outdword:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%eax						# load value
	out		%eax,%dx							# write to port
	ret
