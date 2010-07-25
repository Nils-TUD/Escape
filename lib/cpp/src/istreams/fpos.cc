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

#include <istreams/fpos.h>

namespace std {
	template<class stateT>
	fpos<stateT>::fpos()
		: _st(stateT()), _off(streamoff()) {
	}
	template<class stateT>
	fpos<stateT>::fpos(streamoff off)
		: _st(stateT()), _off(off) {
	}
	template<class stateT>
	fpos<stateT>::~fpos() {
	}

	template<class stateT>
	stateT fpos<stateT>::state() const {
		return _st;
	}
	template<class stateT>
	void fpos<stateT>::state(stateT st) {
		_st = st;
	}

	template<class stateT>
	fpos<stateT>::operator streamoff() const {
		return _off;
	}
	template<class stateT>
	fpos& fpos<stateT>::operator +=(streamoff off) const {
		_off += off;
		return *this;
	}
	template<class stateT>
	fpos& fpos<stateT>::operator -=(streamoff off) const {
		_off -= off;
		return *this;
	}
	template<class stateT>
	fpos fpos<stateT>::operator +(streamoff off) const {
		fpos<stateT> tmp(*this);
		tmp += off;
		return tmp;
	}
	template<class stateT>
	fpos fpos<stateT>::operator -(streamoff off) const {
		fpos<stateT> tmp(*this);
		tmp -= off;
		return tmp;
	}
	template<class stateT>
	streamoff fpos<stateT>::operator -(const fpos& pos) const {
		return _off - pos._off;
	}
}
