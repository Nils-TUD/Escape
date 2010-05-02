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

#ifndef OUTOFMEMORY_H_
#define OUTOFMEMORY_H_

#include <esc/common.h>
#include <exceptions/exception.h>

#define ID_OutOfMemoryException					1

typedef sException sOutOfMemoryException;

/**
 * Constructor for the out-of-memory-exception
 *
 * @param id the exception-id
 * @param line the line
 * @param file the file
 * @return the exception
 */
sOutOfMemoryException *ex_createOutOfMemoryException(s32 id,s32 line,const char *file);

#endif /* OUTOFMEMORY_H_ */
