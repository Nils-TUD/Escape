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
#include <sys/task/elf.h>
#include <esc/lock.h>
#include <esc/sllist.h>
#include "loader.h"
#include "dl-phdr.h"

#define RETURN_ADDRESS(nr) __builtin_extract_return_addr(__builtin_return_address(nr))

static tULock phdrLock;

int dl_iterate_phdr(fCallback callback,void *data) {
	sSLNode *n;
	struct dl_phdr_info info;
	int ret = 0;

	/* Make sure we are alone */
	locku(&phdrLock);

	/* atm, its not possible to add shared objects during runtime. so we don't need to
	 * calculate dlpi_adds and dlpi_subs.
	 */
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		info.dlpi_addr = l->loadAddr;
		info.dlpi_name = l->name;
		info.dlpi_phdr = (const Elf32_Phdr*)l->phdr;
		info.dlpi_phnum = l->phdrNum;
		info.dlpi_adds = 0;	/* atm, always 0 */
		info.dlpi_subs = 0;	/* atm, always 0 */
		info.dlpi_tls_modid = 0;
		info.dlpi_tls_data = NULL;
		info.dlpi_tls_modid = 0;	/* not needed, atm */
		ret = callback(&info,sizeof(struct dl_phdr_info),data);
		if(ret)
			break;
	}

	/* Release the lock */
	unlocku(&phdrLock);

	return ret;
}
