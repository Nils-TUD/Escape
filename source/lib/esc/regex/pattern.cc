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

#include <esc/regex/elements.h>
#include <stdio.h>

#include "pattern.h"

using namespace esc;

void pattern_destroy(void *e) {
	delete reinterpret_cast<Regex::PatternNode*>(e);
}

void *pattern_createGroup(void *l) {
	ElementList<Regex::Element> *list = reinterpret_cast<ElementList<Regex::Element>*>(l);
	return new GroupElement(list);
}

void *pattern_createList(void) {
	return new ElementList<Regex::Element>();
}

void pattern_addToList(void *l,void *e) {
	ElementList<Regex::Element> *list = reinterpret_cast<ElementList<Regex::Element>*>(l);
	Regex::Element *el = reinterpret_cast<Regex::Element*>(e);
	list->add(el);
}

void *pattern_createChar(char c) {
	return new CharElement(c);
}

void *pattern_createDot(void) {
	return new DotElement();
}

void *pattern_createRepeat(void *e,int min,int max) {
	Regex::Element *el = reinterpret_cast<Regex::Element*>(e);
	if(el->type() == Regex::Element::REPEAT)
		yyerror("Unable to repeat a repeat-element");
	if(min < 0 || max <= 0 || max < min)
		yyerror("Invalid repeat specification");
	return new RepeatElement(el,min,max);
}

void *pattern_createCharClass(void *l,bool negate) {
	ElementList<CharClassElement::Range> *list = reinterpret_cast<ElementList<CharClassElement::Range>*>(l);
	return new CharClassElement(list,negate);
}

void pattern_addToCharClassList(void *l,void *elem) {
	ElementList<CharClassElement::Range> *list = reinterpret_cast<ElementList<CharClassElement::Range>*>(l);
	CharClassElement::Range *range = reinterpret_cast<CharClassElement::Range*>(elem);
	list->add(range);
}

void *pattern_createCharClassList() {
	return new ElementList<CharClassElement::Range>();
}

void *pattern_createCharClassElem(char begin,char end) {
	return new CharClassElement::Range(begin,end);
}
