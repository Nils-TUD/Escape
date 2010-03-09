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

#ifndef SWAP_H_
#define SWAP_H_

#include <common.h>

/**
 * Starts the swapping-system. This HAS TO be done with the swapping-thread!
 */
void swap_start(void);

/**
 * Swaps the page with given address for the given process in
 *
 * @param p the process
 * @param addr the address of the page
 * @return true if successfull
 */
bool swap_in(sProc *p,u32 addr);

/**
 * Checks wether there we should swap something out
 */
void swap_check(void);

#endif /* SWAP_H_ */
