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
#include <esc/stream/fstream.h>
#include <stdlib.h>
#include <errno.h>

extern char __progname[];

namespace esc {

extern FStream sin;
extern FStream sout;
extern FStream serr;

static inline void appenderr(esc::OStream &os) {
	if(errno != 0)
		os << ": " << strerror(errno);
	os << "\n";
}

#define errmsg(expr) do {									\
		esc::serr << "[" << __progname << "] " << expr;		\
		appenderr(esc::serr);								\
	}														\
	while(0)

#define exitmsg(expr) do {	 								\
		errmsg(expr); 										\
		exit(EXIT_FAILURE);									\
	}														\
	while(0)

}
