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

#include <istreams/basic_streambuf.h>

namespace std {
	template<class charT,class traits>
	basic_streambuf<charT,traits>::basic_streambuf() {
		_eback = NULL;
		_gptr = NULL;
		_egptr = NULL;
		_pbase = NULL;
		_pptr = NULL;
		_epptr = NULL;
	}
	template<class charT,class traits>
	inline basic_streambuf<charT,traits>::~basic_streambuf() {
	}

	template<class charT,class traits>
	inline basic_streambuf<charT,traits>* basic_streambuf<charT,traits>::pubsetbuf(char_type * s,
			streamsize n) {
		return setbuf(s,n);
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::pos_type basic_streambuf<charT,traits>::pubseekoff(
			off_type off,ios_base::seekdir way,ios_base::openmode which) {
		return seekoff(off,way,which);
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::pos_type basic_streambuf<charT,traits>::pubseekpos(
			pos_type sp,ios_base::openmode which) {
		return seekpos(sp,which);
	}
	template<class charT,class traits>
	inline int basic_streambuf<charT,traits>::pubsync() {
		return sync();
	}

	template<class charT,class traits>
	streamsize basic_streambuf<charT,traits>::in_avail() {
		if(_gptr && _gptr < _egptr)
			return _egptr - _gptr;
		return showmanyc();
	}
	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::snextc() {
		if(sbumpc() == traits::eof())
			return traits::eof();
		return sgetc();
	}
	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::sbumpc() {
		if(!(_gptr && _gptr < _egptr))
			return uflow();
		return traits::to_int_type(*_gptr++);
	}
	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::sgetc() {
		if(!(_gptr && _gptr < _egptr))
			return underflow();
		return traits::to_int_type(*_gptr);
	}
	template<class charT,class traits>
	inline streamsize basic_streambuf<charT,traits>::sgetn(char_type* s,streamsize n) {
		return xsgetn(s,n);
	}

	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::sputbackc(
			char_type c) {
		if(!(_gptr && _gptr > _eback) || !traits::eq(c,_gptr[-1]))
			return pbackfail(traits::to_int_type(c));
		return traits::to_int_type(*--_gptr);
	}
	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::sungetc() {
		if(!(_gptr && _gptr > _eback))
			return pbackfail();
		return traits::to_int_type(*--_gptr);
	}

	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::sputc(char_type c) {
		if(!(_pptr && _pptr < _epptr))
			return overflow(traits::to_int_type(c));
		*_pptr++ = c;
		return traits::to_int_type(c);
	}
	template<class charT,class traits>
	inline streamsize basic_streambuf<charT,traits>::sputn(const char_type* s,streamsize n) {
		return xsputn(s,n);
	}

	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::eback() const {
		return _eback;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::gptr() const {
		return _gptr;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::egptr() const {
		return _egptr;
	}
	template<class charT,class traits>
	inline void basic_streambuf<charT,traits>::gbump(int n) {
		_gptr += n;
	}
	template<class charT,class traits>
	void basic_streambuf<charT,traits>::setg(char_type* gbeg,char_type* gnext,char_type* gend) {
		_eback = ebeg;
		_gptr = gnext;
		_egptr = gend;
	}

	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::pbase() const {
		return _pbase;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::pptr() const {
		return _pptr;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::char_type*
			basic_streambuf<charT,traits>::epptr() const {
		return _epptr;
	}
	template<class charT,class traits>
	inline void basic_streambuf<charT,traits>::pbump(int n) {
		_pptr += n;
	}
	template<class charT,class traits>
	inline void basic_streambuf<charT,traits>::setp(char_type* pbeg,char_type* pend) {
		_pbase = _pptr = pbeg;
		_epptr = pend;
	}

	template<class charT,class traits>
	inline basic_streambuf<charT,traits>* basic_streambuf<charT,traits>::setbuf(char_type * s,
			streamsize n) {
		// default impl does nothing
		return *this;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::pos_type basic_streambuf<charT,traits>::seekoff(
			off_type off,ios_base::seekdir way,ios_base::openmode which) {
		return pos_type(off_type(-1));
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::pos_type basic_streambuf<charT,traits>::seekpos(
			pos_type sp,ios_base::openmode which) {
		return pos_type(off_type(-1));
	}
	template<class charT,class traits>
	inline int basic_streambuf<charT,traits>::sync() {
		return 0;
	}

	template<class charT,class traits>
	inline streamsize basic_streambuf<charT,traits>::showmanyc() {
		return 0;
	}
	template<class charT,class traits>
	streamsize basic_streambuf<charT,traits>::xsgetn(char_type* s,streamsize n) {
		streamsize count = n;
		while(n > 0) {
			int_type pos = sbumpc();
			if(pos == traits::eof())
				break;
			*s++ = pos;
			n--;
		}
		return count - n;
	}
	template<class charT,class traits>
	inline streamsize basic_streambuf<charT,traits>::underflow() {
		return traits::eof();
	}
	template<class charT,class traits>
	typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::uflow() {
		if(underflow() == traits::eof())
			return traits::eof();
		return traits::to_int_type(*_gptr++);
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::pbackfail(
			int_type c) {
		return traits::eof();
	}

	template<class charT,class traits>
	streamsize basic_streambuf<charT,traits>::xsputn(const char_type * s,streamsize n) {
		streamsize count = n;
		while(n > 0) {
			if(sputc(*s++) == traits::eof())
				break;
			n--;
		}
		return count - n;
	}
	template<class charT,class traits>
	inline typename basic_streambuf<charT,traits>::int_type basic_streambuf<charT,traits>::overflow(
			int_type c) {
		return traits::eof();
	}
}
