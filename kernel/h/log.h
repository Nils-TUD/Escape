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

#ifndef LOG_H_
#define LOG_H_

#include <common.h>
#include <stdarg.h>

/**
 * Tells the log that the VFS is usable now
 */
void log_vfsIsReady(void);

/**
 * Formatted output into the logfile
 *
 * @param fmt the format
 */
void log_printf(const char *fmt,...);

/**
 * Like log_printf, but with argument-list
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void log_vprintf(const char *fmt,va_list ap);

#endif /* LOG_H_ */
