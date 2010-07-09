/**
 * $Id: errno.h 400 2010-01-11 20:41:21Z nasmussen $
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

#ifndef ERRNO_H_
#define ERRNO_H_

#include <esc/common.h>
#include <errors.h>

#ifdef __cplusplus
extern "C" {
#endif

/* needed for flex */
#define EINTR	ERR_INTERRUPTED

/**
 * The last error-code
 */
extern int errno;

#ifdef __cplusplus
}
#endif

#endif /* ERRNO_H_ */
