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

#include <sys/common.h>

enum {
	CONF_TIMER_FREQ				= 0,
	CONF_MAX_PROCS				= 1,
	CONF_MAX_FDS				= 2,
	CONF_LOG					= 3,
	CONF_LOG_TO_VGA				= 4,
	CONF_CPU_COUNT				= 6,
	CONF_TICKS_PER_SEC			= 8,
	CONF_LOG_SYSCALLS			= 12,
	CONF_ROOT_DEVICE			= 32,	/* string */
	CONF_SWAP_DEVICE			= 33,	/* string */
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Gets the value of a kernel-configuration
 *
 * @param id the id of the config-value (CONF_*)
 * @return the value or the negative error-code
 */
long sysconf(int id);

/**
 * Gets the value of the kernel-configuration for string-values.
 *
 * @param id the id of the config-value (CONF_*)
 * @param buf the buffer where to store the value
 * @param len the length of the buffer
 * @return 0 on success
 */
int sysconfstr(int id,char *buf,size_t len);

#if defined(__cplusplus)
}
#endif
