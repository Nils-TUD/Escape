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

#include <esc/stream/ostream.h>
#include <vector>
#include <string>

namespace esc {

/**
 * Implements regular expressions that can be used for matching, searching and replacing.
 *
 * Currently, it supports:
 * - dot operator: .
 * - start and end of a string: ^ and $
 * - groups: ( )
 * - repetition: *, + and ?
 * - character classes: [ ] and [^ ]
 * - choices: |
 */
class Regex {
public:
	class Result;
	class Input;

	static const size_t MAX_GROUP_NESTING		= 16;

	Regex() = delete;

	/**
	 * The base-class of all AST classes
	 */
	class PatternNode {
	public:
		explicit PatternNode() {
		}
		PatternNode(const PatternNode &) = delete;
		PatternNode &operator=(const PatternNode &) = delete;
		virtual ~PatternNode() {
		}
	};

	/**
	 * The base-class of all elements, i.e. components of a regex.
	 */
	class Element : public PatternNode {
	public:
		enum Type {
			CHAR,
			CHARCLASS,
			DOT,
			REPEAT,
			GROUP,
			CHOICE
		};

		explicit Element(Type type) : PatternNode(), _type(type) {
		}

		Type type() const {
			return _type;
		}

		virtual bool match(Result *res,Input &in) const = 0;
		virtual void print(esc::OStream &os,int indent) const = 0;

		friend esc::OStream &operator<<(esc::OStream &os,const Element &e) {
			e.print(os,0);
			return os;
		}

	private:
		Type _type;
	};

	/**
	 * Represents the string that should be matched against a regex.
	 */
	class Input {
	public:
		explicit Input(const std::string &str,size_t pos = 0) : _pos(pos),_str(str) {
		}

		std::string substr(size_t begin,size_t length) const {
			return _str.substr(begin,length);
		}
		size_t pos() const {
			return _pos;
		}
		char peek() const {
			if(_pos < _str.length())
				return _str[_pos];
			return '\0';
		}
		char get() {
			if(_pos < _str.length())
				return _str[_pos++];
			return '\0';
		}

	private:
		size_t _pos;
		const std::string &_str;
	};

	/**
	 * Captures the result of a match/search.
	 */
	class Result {
		friend class GroupElement;
		friend class Regex;

	public:
		explicit Result() : _success(false), _matches() {
		}
		explicit Result(size_t groups) : _success(false), _matches(groups) {
		}

		/**
		 * @return true if it matched
		 */
		bool matched() const {
			return _success;
		}
		/**
		 * @return the number of found groups
		 */
		size_t groups() const {
			return _matches.size();
		}
		/**
		 * @param i the group number
		 * @return the group <i>
		 */
		const std::string &get(size_t i) const {
			return _matches[i];
		}

	private:
		void set(size_t id,const std::string &str) {
			_matches[id] = str;
		}
		void setSuccess(bool succ) {
			_success = succ;
		}

		bool _success;
		std::vector<std::string> _matches;
	};

	/**
	 * Represents a pattern that can be used for matching, searching and replacing.
	 */
	class Pattern {
	public:
		explicit Pattern(Element *root,int flags) : _flags(flags),_root(root) {
		}
		Pattern(const Pattern&) = delete;
		Pattern &operator=(const Pattern&) = delete;
		Pattern(Pattern &&p) : _root(p._root) {
			p._root = NULL;
		}
		Pattern &operator=(Pattern &&p) {
			if(&p != this) {
				_root = p._root;
				p._root = NULL;
			}
			return *this;
		}
		virtual ~Pattern() {
			delete _root;
		}

		int flags() const {
			return _flags;
		}
		const Element *root() const {
			return _root;
		}

		friend esc::OStream &operator<<(esc::OStream &os,const Pattern &p);

	private:
		int _flags;
		Element *_root;
	};

	/**
	 * Compiles the given regular expression into a pattern, i.e. an object tree, that can be used
	 * for matching, searching and replacing.
	 *
	 * @param regex the regular expression
	 * @return the pattern
	 * @throws runtime_error if <regex> is ill-formed
	 */
	static Pattern compile(const std::string &regex);

	/**
	 * Compiles <regex> into a pattern, searches for <regex> in <str> and returns the result.
	 *
	 * @param regex the regular expression
	 * @param str the string to search in
	 * @return the result
	 * @throws runtime_error if <regex> is ill-formed
	 */
	static Result search(const std::string &regex,const std::string &str) {
		return search(compile(regex),str);
	}

	/**
	 * Searches for <pattern> in <str> and returns the result.
	 *
	 * @param pattern the pattern
	 * @param str the string to search in
	 * @return the result
	 */
	static Result search(const Pattern &pattern,const std::string &str);

	/**
	 * Compiles <regex> into a pattern and tests whether <regex> matches <str>.
	 *
	 * @param regex the regular expression
	 * @param str the string to test
	 * @return true if so
	 * @throws runtime_error if <regex> is ill-formed
	 */
	static bool matches(const std::string &regex,const std::string &str) {
		return matches(compile(regex),str);
	}

	/**
	 * Tests whether <pattern> matches <str>.
	 *
	 * @param pattern the pattern
	 * @param str the string to match
	 * @return true if so
	 */
	static bool matches(const Pattern &pattern,const std::string &str);

	/**
	 * Compiles <regex> into a pattern, searches for it in <str> and replaces <str> with <repl>.
	 * You can specify groups in <repl> with $<no>.
	 *
	 * @param regex the regular expression
	 * @param str the string to search in
	 * @param repl the string to replace it with
	 * @return the replaced string
	 * @throws runtime_error if <regex> is ill-formed
	 */
	static std::string replace(const std::string &regex,const std::string &str,const std::string &repl) {
		return replace(compile(regex),str,repl);
	}

	/**
	 * Searches for <pattern> in <str> and replaces <str> with <repl>.
	 * You can specify groups in <repl> with $<no>.
	 *
	 * @param pattern the pattern
	 * @param str the string to search in
	 * @param repl the string to replace it with
	 * @return the replaced string
	 */
	static std::string replace(const Pattern &pattern,const std::string &str,const std::string &repl);

private:
	static Regex::Result test(const Pattern &p,const std::string &str,size_t pos);
};

}
