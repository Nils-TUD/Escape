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

.global inByte
.global inWord
.global inDWord
.global outByte
.global outWord
.global outDWord

# uint8_t inByte(uint16_t port)
.type inByte, @function
inByte:
	mov		4(%esp),%dx							# load port
	in		%dx,%al								# read from port
	ret

# uint16_t inWord(uint16_t port)
.type inWord, @function
inWord:
	mov		4(%esp),%dx							# load port
	in		%dx,%ax								# read from port
	ret

# uint32_t inDWord(uint16_t port)
.type inDWord, @function
inDWord:
	mov		4(%esp),%dx							# load port
	in		%dx,%eax							# read from port
	ret

# void outByte(uint16_t port,uint8_t val)
.type outByte, @function
outByte:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%al							# load value
	out		%al,%dx								# write to port
	ret

# void outWord(uint16_t port,uint16_t val)
.type outWord, @function
outWord:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%ax							# load value
	out		%ax,%dx								# write to port
	ret

# void outDWord(uint16_t port,uint32_t val)
.type outDWord, @function
outDWord:
	mov		4(%esp),%dx							# load port
	mov		8(%esp),%eax						# load value
	out		%eax,%dx							# write to port
	ret
