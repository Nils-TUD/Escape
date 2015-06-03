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

class GroupElement;

class Regex {
public:
	static const size_t MAX_GROUP_NESTING		= 16;

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

	class PatternNode {
	public:
		explicit PatternNode() {
		}
		PatternNode(const PatternNode &) = delete;
		PatternNode &operator=(const PatternNode &) = delete;
		virtual ~PatternNode() {
		}
	};

	class Result {
		friend class GroupElement;
		friend class Regex;

	public:
		explicit Result() : _success(false), _matches() {
		}
		explicit Result(size_t groups) : _success(false), _matches(groups) {
		}

		bool matched() const {
			return _success;
		}
		size_t groups() const {
			return _matches.size();
		}
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

	static Pattern compile(const std::string &pattern);
	static Result search(const Pattern &p,const std::string &str);

	static bool matches(const std::string &pattern,const std::string &str) {
		return matches(compile(pattern),str);
	}
	static bool matches(const Pattern &p,const std::string &str);

	static std::string replace(const std::string &pattern,const std::string &str,const std::string &repl) {
		return replace(compile(pattern),str,repl);
	}
	static std::string replace(const Pattern &p,const std::string &str,const std::string &repl);

private:
	static Regex::Result test(const Pattern &p,const std::string &str,size_t pos);
};

}
