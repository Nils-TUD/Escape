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

#if defined(__i586__)
#	include <sys/arch/i586/intrptstackframe.h>
#elif defined(__x86_64__)
#	include <sys/arch/x86_64/intrptstackframe.h>
#elif defined(__eco32__)
#	include <sys/arch/eco32/intrptstackframe.h>
#elif defined(__mmix__)
#	include <sys/arch/mmix/intrptstackframe.h>
#endif
