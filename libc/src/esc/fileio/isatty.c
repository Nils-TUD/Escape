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

#include <esc/common.h>
#include <esc/fileio.h>
#include <esc/io.h>

int isatty(int fd) {
	/* a way to determine wether its a vterm is to try an ioctl-command that every vterm
	 * should implement (and that doesn't change anything) */
	/* other drivers should respond with ERR_UNSUPPORTED_OP */
	sIoCtlSize consSize;
	return ioctl(fd,IOCTL_VT_GETSIZE,&consSize,sizeof(consSize)) >= 0;
}
