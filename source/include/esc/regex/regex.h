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

class Regex {
public:
	static const size_t MAX_GROUP_NESTING		= 16;

	class Input {
	public:
		explicit Input(const std::string &str) : _pos(),_str(str) {
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

		virtual bool match(Input &in) const = 0;
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
		explicit Pattern(Element *root) : _root(root) {
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

		Element *root() {
			return _root;
		}

		friend esc::OStream &operator<<(esc::OStream &os,const Pattern &p);

	private:
		Element *_root;
	};

	static Pattern compile(const std::string &pattern);

	static bool matches(const std::string &pattern,const std::string &str) {
		Pattern p = compile(pattern);
		return matches(p,str);
	}
	static bool matches(Pattern &p,const std::string &str);
	static std::string replace(const std::string &pattern,const std::string &str,const std::string &repl) {
		Pattern p = compile(pattern);
		return replace(p,str,repl);
	}
	static std::string replace(Pattern &p,const std::string &str,const std::string &repl);
};

}
