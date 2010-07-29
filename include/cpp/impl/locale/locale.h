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

#ifndef LOCALE_H_
#define LOCALE_H_

namespace std {
	class locale {
	public:
		class facet {
		protected:
			explicit facet(size_t refs = 0);
			virtual ~facet();
		private:
			facet(const facet &);			// not defined
			void operator =(const facet&);	// not defined
		};

		class id {
		public:
			id();
		private:
			id(const id&);					// not defined
			void operator =(const id&);		// not defined
		};

		typedef int category;

		// values assigned here are for exposition only
		static const category
			none = 0,
			collate = 0x010,
			ctype = 0x020,
			monetary = 0x040,
			numeric = 0x080,
			time = 0x100,
			messages = 0x200,
			all = collate | ctype | monetary | numeric | time | messages;

		// construct/copy/destroy:
		/**
		 * Default constructor: a snapshot of the current global locale.
		 * Constructs a copy of the argument last passed to locale::global(locale&), if it has
		 * been called; else, the resulting facets have virtual function semantics identical to
		 * those of locale::classic().
		 */
		locale() throw ();
		/**
		 * Constructs a locale which is a copy of other .
		 */
		locale(const locale& other) throw ();
		/**
		 * Constructs a locale using standard C locale names, e.g. "POSIX". The resulting locale
		 * implements semantics defined to be associated with that name.
		 * The set of valid string argument values is "C", "", and any implementation-defined values
		 *
		 * @throws runtime_error if the argument is not valid, or is null.
		 */
		explicit locale(const char* std_name);
		/**
		 * Constructs a locale as a copy of other except for the facets identified by the category
		 * argument, which instead implement the same semantics as locale(std_name ).
		 * @throws runtime_error if the argument is not valid, or is null.
		 */
		locale(const locale& other,const char* std_name,category);
		/**
		 * Constructs a locale incorporating all facets from the first argument except that of
		 * type Facet, and installs the second argument as the remaining facet. If f is null, the
		 * resulting object is a copy of other .
		 */
		template<class Facet> locale(const locale& other,Facet * f);
		/**
		 * Constructs a locale incorporating all facets from the first argument except those that
		 * implement cats, which are instead incorporated from the second argument.
		 */
		locale(const locale& other,const locale& one,category);
		/**
		 * A non-virtual destructor that throws no exceptions.
		 */
		~locale() throw ();

		// non-virtual
		/**
		 * Creates a copy of other , replacing the current value.
		 * @return *this
		 */
		const locale& operator =(const locale& other) throw ();
		/**
		 * Constructs a locale incorporating all facets from *this except for that one facet
		 * of other that is identified by Facet. The resulting locale has no name.
		 * @return the newly created locale.
		 * @throws runtime_error if has_facet<Facet>(other) is false.
		 */
		template<class Facet> locale combine(const locale& other) const;

		// locale operations:
		/**
		 * @return The name of *this, if it has one; otherwise, the string "*". If *this has a name,
		 * then locale(name().c_str()) is equivalent to *this. Details of the contents of the
		 * resulting string are otherwise implementation-defined.
		 */
		basic_string<char> name() const;
		/**
		 * @return true if both arguments are the same locale, or one is a copy of the other, or
		 * each has a name and the names are identical; false otherwise.
		 */
		bool operator ==(const locale & other) const;
		/**
		 * @return the result of the expression: !(*this == other ).
		 */
		bool operator !=(const locale & other) const;
		/**
		 * Compares two strings according to the collate<charT> facet.
		 * This member operator template (and therefore locale itself) satisfies requirements for
		 * a comparator predicate template argument (clause 25) applied to strings.
		 * @return The result of the following expression:
		 * 	use_facet<collate<charT> >(*this).compare(s1.data(),s1.data()+s1.size(),
		 * 	s2.data(),s2.data()+s2.size()) < 0;
		 */
		template<class charT,class Traits,class Allocator>
		bool operator ()(const basic_string<charT,Traits,Allocator>& s1,const basic_string<charT,
				Traits,Allocator>& s2) const;

		// global locale objects:
		/**
		 * Sets the global locale to its argument.
		 * Causes future calls to the constructor locale() to return a copy of the argument. If the
		 * argument has a name, does std::setlocale(LC_ALL,loc.name().c_str()); otherwise, the
		 * effect on the C locale, if any, is implementation-defined. No library function other
		 * than locale::global() shall affect the value returned by locale().
		 * @return The previous value of locale().
		 */
		static locale global(const locale &);
		/**
		 * The "C" locale.
		 * @return A locale that implements the classic "C" locale semantics, equivalent to the
		 * value locale("C").
		 */
		static const locale & classic();
	};
}

#endif /* LOCALE_H_ */
