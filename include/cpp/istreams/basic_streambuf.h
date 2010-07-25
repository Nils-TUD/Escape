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

#ifndef BASIC_STREAMBUF_H_
#define BASIC_STREAMBUF_H_

#include <stddef.h>

namespace std {
	template<class charT,class traits = char_traits<charT> >
	class basic_streambuf {
	public:
		typedef charT char_type;
		typedef typename traits::int_type int_type;
		typedef typename traits::pos_type pos_type;
		typedef typename traits::off_type off_type;
		typedef traits traits_type;

		/**
		 * Destructor (does nothing)
		 */
		virtual ~basic_streambuf();

		/**
		 * @return setbuf(s , n ).
		 */
		basic_streambuf<char_type,traits>* pubsetbuf(char_type * s,streamsize n);
		/**
		 * @return seekoff(off , way , which ).
		 */
		pos_type pubseekoff(off_type off,ios_base::seekdir way,ios_base::openmode which =
				ios_base::in | ios_base::out);
		/**
		 * @return seekpos(sp , which ).
		 */
		pos_type pubseekpos(pos_type sp,ios_base::openmode which = ios_base::in | ios_base::out);
		/**
		 * @return sync().
		 */
		int pubsync();

		/**
		 * @return if a read position is available, returns egptr() - gptr(). Otherwise returns
		 * 	showmanyc()
		 */
		streamsize in_avail();
		/**
		 * Calls sbumpc().
		 *
		 * @return if that function returns traits::eof(), returns traits::eof(). Otherwise,
		 * 	returns sgetc().
		 */
		int_type snextc();
		/**
		 * @return if the input sequence read position is not available, returns uflow(). Otherwise,
		 * 	returns traits::to_int_type(*gptr()) and increments the next pointer for the input
		 * 	sequence.
		 */
		int_type sbumpc();
		/**
		 * @return if the input sequence read position is not available, returns underflow().
		 * 	Otherwise, returns traits::to_int_type(*gptr()).
		 */
		int_type sgetc();
		/**
		 * @return xsgetn(s , n ).
		 */
		streamsize sgetn(char_type* s,streamsize n);

		/**
		 * @return if the input sequence putback position is not available, or if
		 * 	traits::eq(c ,gptr()[-1]) is false, returns pbackfail(traits::to_int_type(c )).
		 * 	Otherwise, decrements the next pointer for the input sequence and returns
		 * 	traits::to_int_type(*gptr()).
		 */
		int_type sputbackc(char_type c);
		/**
		 * @return if the input sequence putback position is not available, returns pbackfail().
		 * 	Otherwise, decrements the next pointer for the input sequence and returns
		 * 	traits::to_int_type(*gptr()).
		 */
		int_type sungetc();

		/**
		 * @return if the output sequence write position is not available, returns
		 * 	overflow(traits::to_int_type(c )). Otherwise, stores c at the next pointer for the
		 * 	output sequence, increments the pointer, and returns traits::to_int_type(c ).
		 */
		int_type sputc(char_type c);
		/**
		 * @return xsputn(s ,n ).
		 */
		streamsize sputn(const char_type* s,streamsize n);

	protected:
		/**
		 * Constructs an object of class basic_streambuf<charT,traits> and initializes:
		 * - all its pointer member objects to null pointers,
		 * - the getloc() member to a copy the global locale, locale(), at the time of construction.
		 * The default constructor is protected for class basic_streambuf to assure that only
		 * objects for classes derived from this class may be constructed.
		 */
		basic_streambuf();

		/**
		 * @return the beginning pointer for the input sequence.
		 */
		char_type* eback() const;
		/**
		 * @return the next pointer for the input sequence.
		 */
		char_type* gptr() const;
		/**
		 * @return the end pointer for the input sequence.
		 */
		char_type* egptr() const;
		/**
		 * Adds n to the next pointer for the input sequence.
		 */
		void gbump(int n);
		/**
		 * Sets eback() to gbeg, gptr() to gnext and egptr() to gend.
		 */
		void setg(char_type* gbeg,char_type* gnext,char_type* gend);

		/**
		 * @return the beginning pointer for the output sequence.
		 */
		char_type* pbase() const;
		/**
		 * @return the next pointer for the output sequence.
		 */
		char_type* pptr() const;
		/**
		 * @return the end pointer for the output sequence.
		 */
		char_type* epptr() const;
		/**
		 * Adds n to the next pointer for the output sequence.
		 */
		void pbump(int n);
		/**
		 * Sets pbase() to pbeg, pptr() to pbeg and epptr() to pend.
		 */
		void setp(char_type* pbeg,char_type* pend);

		/**
		 * Influences stream buffering in a way that is defined separately for each class
		 * derived from basic_streambuf in this clause (27.7.1.3, 27.8.1.4).
		 * Default behavior: Does nothing. Returns this.
		 */
		virtual basic_streambuf<char_type,traits>* setbuf(char_type * s,streamsize n);
		/**
		 * Alters the stream positions within one or more of the controlled sequences in a way
		 * that is defined separately for each class derived from basic_streambuf in this clause
		 * (27.7.1.3, 27.8.1.4).
		 * Default behavior: Returns pos_type(off_type(-1)).
		 */
		virtual pos_type seekoff(off_type off,ios_base::seekdir way,
				ios_base::openmode which = ios_base::in | ios_base::out);
		/**
		 * Alters the stream positions within one or more of the controlled sequences in a way that
		 * is defined separately for each class derived from basic_streambuf in this clause
		 * (27.7.1, 27.8.1.1).
		 * Default behavior: Returns pos_type(off_type(-1)).
		 */
		virtual pos_type seekpos(pos_type sp,
				ios_base::openmode which = ios_base::in | ios_base::out);
		/**
		 * Synchronizes the controlled sequences with the arrays. That is, if pbase() is non-null
		 * the characters between pbase() and pptr() are written to the controlled sequence.
		 * The pointers may then be reset as appropriate.
		 * Returns: -1 on failure. What constitutes failure is determined by each derived class
		 * Default behavior: Returns zero.
		 */
		virtual int sync();

		/**
		 * Returns: an estimate of the number of characters available in the sequence, or -1. If
		 * it returns a positive value, then successive calls to underflow() will not return
		 * traits::eof() until at least that number of characters have been extracted from the
		 * stream. If showmanyc() returns -1, then calls to underflow() or uflow() will fail.
		 * Default behavior: Returns zero.
		 * Remarks: Uses traits::eof().
		 */
		virtual streamsize showmanyc();
		/**
		 * Effects: Assigns up to n characters to successive elements of the array whose first
		 * element is designated by s . The characters assigned are read from the input sequence
		 * as if by repeated calls to sbumpc(). Assigning stops when either n characters have
		 * been assigned or a call to sbumpc() would return traits::eof().
		 * Returns: The number of characters assigned.
		 * Remarks: Uses traits::eof().
		 */
		virtual streamsize xsgetn(char_type * s,streamsize n);
		/**
		 * Remarks: The public members of basic_streambuf call this virtual function only if
		 * gptr() is null or gptr() >= egptr()
		 * Returns: traits::to_int_type(c ), where c is the first character of the pending sequence,
		 * without moving the input sequence position past it. If the pending sequence is null then
		 * the function returns traits::eof() to indicate failure.
		 * Default behavior: Returns traits::eof().
		 */
		virtual streamsize underflow();
		/**
		 * Requires: The constraints are the same as for underflow(), except that the result
		 * character is transferred from the pending sequence to the backup sequence, and the
		 * pending sequence may not be empty before the transfer.
		 * Default behavior: Calls underflow(). If underflow() returns traits::eof(), returns
		 * traits::eof(). Otherwise, returns the value of traits::to_int_type(*gptr()) and
		 * increment the value of the next pointer for the input sequence.
		 * Returns: traits::eof() to indicate failure.
		 */
		virtual int_type uflow();
		/**
		 * Tries to back up the input sequence
		 * Default behavior: Returns traits::eof().
		 */
		virtual int_type pbackfail(int_type c = traits::eof());

		/**
		 * Writes up to n characters to the output sequence as if by repeated calls to sputc(c ).
		 * The characters written are obtained from successive elements of the array whose first
		 * element is designated by s . Writing stops when either n characters have been written
		 * or a call to sputc(c ) would return traits::eof().
		 * @return the number of characters written.
		 */
		virtual streamsize xsputn(const char_type * s,streamsize n);
		/**
		 * Flushes out the pending sequence (pbase() .. pptr()), followed by c, if c is not EOF
		 */
		virtual int_type overflow(int_type c = traits::eof());

	private:
		// output sequence
		char_type *_pbase;
		char_type *_pptr;
		char_type *_epptr;
		// input sequence
		char_type *_eback;
		char_type *_gptr;
		char_type *_egptr;
	};
}

#include "../../../lib/cpp/src/istreams/basic_streambuf.cc"

#endif /* BASIC_STREAMBUF_H_ */
