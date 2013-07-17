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

#pragma once

#include <sys/common.h>

#define CONF_TIMER_FREQ			0
#define CONF_MAX_PROCS			1
#define CONF_MAX_FDS			2
#define CONF_SWAP_DEVICE		4
#define CONF_LOG				5
#define CONF_PAGE_SIZE			6
#define CONF_LINEBYLINE			7
#define CONF_LOG2SCR			8
#define CONF_CPU_COUNT			9
#define CONF_SMP				10

/**
 * Parses the given boot-parameter and sets the configuration-options correspondingly
 *
 * @param argc the number of args
 * @param argv the arguments
 */
void conf_parseBootParams(int argc,const char *const *argv);

/**
 * Returns the value for the given configuration-string-value
 *
 * @param id the config-id
 * @return the value or NULL
 */
const char *conf_getStr(int id);

/**
 * Returns the value for the given configuration-value
 *
 * @param id the config-id
 * @return the value or < 0 if an error occurred
 */
long conf_get(int id);
