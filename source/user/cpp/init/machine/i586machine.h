/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef I586MACHINE_H_
#define I586MACHINE_H_

#include <esc/common.h>
#include "machine.h"

class i586Machine : public Machine {
private:
	static const uint16_t PORT_KB_DATA	= 0x60;
	static const uint16_t PORT_KB_CTRL	= 0x64;

public:
	i586Machine()
		: Machine() {
	};
	virtual ~i586Machine() {
	};

	virtual void reboot(Progress &pg);
	virtual void shutdown(Progress &pg);
};

#endif /* I586MACHINE_H_ */
