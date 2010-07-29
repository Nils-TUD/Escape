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

#ifndef ICTYPE_H_
#define ICTYPE_H_

#include <stddef.h>

namespace std {
	class ctype_base {
	public:
		typedef unsigned long mask;
		// numeric values are for exposition only.
		static const mask space = 1 << 0;
		static const mask print = 1 << 1;
		static const mask cntrl = 1 << 2;
		static const mask upper = 1 << 3;
		static const mask lower = 1 << 4;
		static const mask alpha = 1 << 5;
		static const mask digit = 1 << 6;
		static const mask punct = 1 << 7;
		static const mask xdigit = 1 << 8;
		static const mask alnum = alpha | digit;
		static const mask graph = alnum | punct;
	};

	template<class charT>
	class ctype: public locale::facet,public ctype_base {
	public:
		typedef charT char_type;
		explicit ctype(size_t refs = 0);
		bool is(mask m,charT c) const;
		const charT* is(const charT* low,const charT* high,mask* vec) const;
		const charT* scan_is(mask m,const charT* low,const charT* high) const;
		const charT* scan_not(mask m,const charT* low,const charT* high) const;
		charT toupper(charT c) const;
		const charT* toupper(charT* low,const charT* high) const;
		charT tolower(charT c) const;
		const charT* tolower(charT* low,const charT* high) const;
		charT widen(char c) const;
		const char* widen(const char* low,const char* high,charT* to) const;
		char narrow(charT c,char dfault) const;
		const charT* narrow(const charT* low,const charT* high,char dfault,char* to) const;

		static locale::id id;
	protected:
		~ ctype();
		// virtual
		virtual bool do_is(mask m,charT c) const;
		virtual const charT* do_is(const charT* low,const charT* high,mask* vec) const;
		virtual const charT* do_scan_is(mask m,const charT* low,const charT* high) const;
		virtual const charT* do_scan_not(mask m,const charT* low,const charT* high) const;
		virtual charT do_toupper(charT) const;
		virtual const charT* do_toupper(charT* low,const charT* high) const;
		virtual charT do_tolower(charT) const;
		virtual const charT* do_tolower(charT* low,const charT* high) const;
		virtual charT do_widen(char) const;
		virtual const char* do_widen(const char * low,const char* high,charT* dest) const;
		virtual char do_narrow(charT,char dfault) const;
		virtual const charT* do_narrow(const charT * low,const charT* high,char dfault,
				char* dest) const;
	};
}

#endif /* ICTYPE_H_ */
