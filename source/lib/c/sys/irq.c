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
#include <sys/irq.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <stdio.h>

int semcrtirq(int irq,const char *name,uint64_t *msiaddr,uint32_t *msival) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/sys/irq/%d",irq);
	int fd = open(path,O_MSGS);
	if(fd < 0)
		return fd;
	int res = fsemcrtirq(fd,name,msiaddr,msival);
	close(fd);
	return res;
}
