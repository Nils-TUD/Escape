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

#include <esc/regex/regex.h>

namespace esc {

class ElementList : public Regex::PatternNode {
public:
	typedef typename std::vector<Regex::Element*>::iterator iterator;
	typedef typename std::vector<Regex::Element*>::const_iterator const_iterator;

	explicit ElementList(size_t id) : Regex::PatternNode(), _id(id), _elems() {
	}
	virtual ~ElementList() {
		for(size_t i = 0; i < _elems.size(); ++i)
			delete _elems[i];
	}

	size_t id() const {
		return _id;
	}
	bool empty() const {
		return _elems.empty();
	}
	const_iterator begin() const {
		return _elems.begin();
	}
	const_iterator end() const {
		return _elems.end();
	}

	Regex::Element *add(Regex::Element *e) {
		_elems.push_back(e);
		return e;
	}

	void print(const char *name,esc::OStream &os,int indent) const {
		os << name << "[\n";
		for(auto it = _elems.begin(); it != _elems.end(); ++it) {
			os << fmt("",indent + 2);
			(*it)->print(os,indent + 2);
			os << "\n";
		}
		os << fmt("",indent) << "]";
	}

private:
	size_t _id;
	std::vector<Regex::Element*> _elems;
};

class CharElement : public Regex::Element {
public:
	explicit CharElement(char c) : Regex::Element(CHAR),_c(c) {
	}

	virtual bool match(Regex::Result *,Regex::Input &in) const override {
		if(in.peek() == _c) {
			in.get();
			return true;
		}
		return false;
	}

	virtual void print(esc::OStream &os,int) const override {
		os << "CharElement[" << _c << "]";
	}

private:
	char _c;
};

class CharClassElement : public Regex::Element {
public:
	struct Range : public Regex::Element {
		explicit Range(char _begin,char _end) : Element(CHARCLASS_RANGE), begin(_begin),end(_end) {
		}

		virtual bool match(Regex::Result *,Regex::Input &in) const override {
			if(in.peek() >= begin && in.peek() <= end) {
				in.get();
				return true;
			}
			return false;
		}

		virtual void print(esc::OStream &os,int) const override {
			if(begin == end)
				os << begin;
			else
				os << begin << '-' << end;
		}

		char begin;
		char end;
	};

	explicit CharClassElement(const ElementList *elems,bool negate)
		: Regex::Element(CHARCLASS),_negate(negate),_elems(elems) {
	}
	virtual ~CharClassElement() {
		delete _elems;
	}

	virtual bool match(Regex::Result *res,Regex::Input &in) const override {
		if(in.peek() != '\0') {
			bool matched = inRanges(res,in);
			if(matched && _negate) {
				in.put();
				return false;
			}
			if(!matched && _negate) {
				in.get();
				return true;
			}
			return matched;
		}
		return false;
	}

	virtual void print(esc::OStream &os,int) const override {
		os << "CharClassElement[";
		if(_negate)
			os << '^';
		for(auto &e : *_elems)
			e->print(os,0);
		os << "]";
	}

private:
	bool inRanges(Regex::Result *res,Regex::Input &in) const {
		for(auto &e : *_elems) {
			if(e->match(res,in))
				return true;
		}
		return false;
	}

	bool _negate;
	const ElementList *_elems;
};

class DotElement : public Regex::Element {
public:
	explicit DotElement() : Regex::Element(DOT) {
	}

	virtual bool match(Regex::Result *,Regex::Input &in) const override {
		if(in.peek() != '\0') {
			in.get();
			return true;
		}
		return false;
	}

	virtual void print(esc::OStream &os,int) const override {
		os << "DotElement[]";
	}
};

class RepeatElement : public Regex::Element {
public:
	explicit RepeatElement(Regex::Element *e,int min,int max)
		: Regex::Element(REPEAT),_e(e),_min(min),_max(max) {
	}
	virtual ~RepeatElement() {
		delete _e;
	}

	virtual bool match(Regex::Result *res,Regex::Input &in) const override {
		int num = 0;
		while(_e->match(res,in)) {
			num++;
			if(num >= _max)
				return true;
		}
		return num >= _min;
	}

	virtual void print(esc::OStream &os,int indent) const override {
		os << "RepeatElement[";
		_e->print(os,indent);
		os << "; min=" << _min << ",max=" << _max << "]";
	}

private:
	Regex::Element *_e;
	int _min;
	int _max;
};

class ChoiceElement : public Regex::Element {
public:
	explicit ChoiceElement(const ElementList *list) : Regex::Element(CHOICE), _list(list) {
	}
	virtual ~ChoiceElement() {
		delete _list;
	}

	virtual bool match(Regex::Result *res,Regex::Input &in) const override {
		for(auto &e : *_list) {
			if(e->match(res,in))
				return true;
		}
		return false;
	}

	virtual void print(esc::OStream &os,int indent) const override {
		_list->print("ChoiceElement",os,indent);
	}

private:
	const ElementList *_list;
};

class GroupElement : public Regex::Element {
public:
	explicit GroupElement(const ElementList *list)
		: Regex::Element(GROUP), _list(list) {
	}
	virtual ~GroupElement() {
		delete _list;
	}

	virtual bool match(Regex::Result *res,Regex::Input &in) const override {
		if(_list->empty())
			return in.peek() == '\0';

		if(res) {
			size_t begin = in.pos();
			for(auto &e : *_list) {
				if(!e->match(res,in))
					return false;
			}
			size_t end = in.pos();
			res->set(_list->id(),in.substr(begin,end - begin));
		}
		else {
			for(auto &e : *_list) {
				if(!e->match(NULL,in))
					return false;
			}
		}
		return true;
	}

	virtual void print(esc::OStream &os,int indent) const override {
		_list->print("GroupElement",os,indent);
	}

private:
	const ElementList *_list;
};

}
