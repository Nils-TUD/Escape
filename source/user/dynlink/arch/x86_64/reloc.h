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

#include "../../setup.h"

static inline bool perform_reloc(sSharedLib *l,int type,size_t offset,uintptr_t *ptr,
		uintptr_t symval,size_t addend) {
	switch(type) {
		case R_X86_64_JUMP_SLOT:
		case R_X86_64_GLOB_DAT:
		case R_X86_64_64:
			*ptr = symval + l->loadAddr + addend;
			return true;

		case R_X86_64_RELATIVE:
			*ptr = l->loadAddr + addend;
			return true;

		case R_X86_64_32:
			*(uint32_t*)ptr = symval + l->loadAddr + addend;
			return true;

		case R_X86_64_PC32:
			*ptr = symval - offset - 4 + addend;
			return true;
	}
	return false;
}
