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

#ifndef FPOS_H_
#define FPOS_H_

#include <stddef.h>
#include <istreams/ios_types.h>

namespace std {
	template<class stateT>
	class fpos {
	public:
		fpos();
		fpos(streamoff off);
		~fpos();

		stateT state() const;
		void state(stateT st);

		operator streamoff() const;
	    fpos& operator+=(streamoff off);
		fpos& operator-=(streamoff off);
		fpos operator+(streamoff off) const;
		fpos operator-(streamoff off) const;
		streamoff operator-(const fpos& pos) const;
	private:
		stateT _st;
		streamoff _off;
	};
}

#include "../../../lib/cpp/src/istreams/fpos.cc"

#endif /* FPOS_H_ */
