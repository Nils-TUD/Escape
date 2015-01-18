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

#include <dbg/console.h>
#include <dbg/kb.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <task/thread.h>
#include <common.h>
#include <interrupts.h>
#include <ksymbols.h>
#include <stdarg.h>
#include <string.h>
#include <util.h>
#include <video.h>

void Util::printUserStateOf(A_UNUSED OStream &os,A_UNUSED const Thread *t) {
}

void Util::printUserState(A_UNUSED OStream &os) {
}
