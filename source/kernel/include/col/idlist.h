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

#include <common.h>
#include <col/dlist.h>
#include <col/ilist.h>

/**
 * An indirect, double linked list. That is, the elements are not inherited from a class that gives
 * us a next-pointer, but we have nodes that form the list and the nodes contain a pointer to the
 * element. This allows us a add an element to multiple lists.
 */
template<class T>
class IDList : public IList<DList,T> {
public:
	/**
	 * Constructor. Creates an empty list
	 */
	explicit IDList() : IList<DList,T>() {
	}
};
