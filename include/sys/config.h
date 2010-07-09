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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <sys/common.h>

#define CONF_TIMER_FREQ			0
#define CONF_MAX_PROCS			1
#define CONF_MAX_THREADS		2
#define CONF_MAX_FDS			3
#define CONF_BOOT_VIDEOMODE		4
#define CONF_SWAP_DEVICE		5

#define CONF_VIDMODE_VGATEXT	0
#define CONF_VIDMODE_VESATEXT	1

/**
 * Parses the given boot-parameter and sets the configuration-options correspondingly
 *
 * @param params the parameter
 */
void conf_parseBootParams(const char *params);

/**
 * Returns the value for the given configuration-string-value
 *
 * @param id the config-id
 * @return the value or NULL
 */
const char *conf_getStr(u32 id);

/**
 * Returns the value for the given configuration-value
 *
 * @param id the config-id
 * @return the value or < 0 if an error occurred
 */
s32 conf_get(u32 id);

#endif /* CONFIG_H_ */
