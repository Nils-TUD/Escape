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

#include <esc/common.h>
#include "node.h"

#define REDIR_NO		0
#define REDIR_ERR2OUT	1
#define REDIR_OUT2ERR	2

typedef struct {
	uchar type;
} sRedirFd;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates an redirect-fd-node with the condition, then- and else-list
 *
 * @param type the redirect-type
 * @return the created node
 */
sASTNode *ast_createRedirFd(uchar type);

/**
 * Prints this redirfd
 *
 * @param s the redirfd
 * @param layer the layer
 */
void ast_printRedirFd(sRedirFd *s,uint layer);

/**
 * Destroys the given redirfd (should be called from ast_destroy() only!)
 *
 * @param n the redirfd
 */
void ast_destroyRedirFd(sRedirFd *n);

#if defined(__cplusplus)
}
#endif
