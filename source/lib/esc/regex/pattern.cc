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

extern size_t regex_groups;

void pattern_destroy(void *e) {
	delete reinterpret_cast<Regex::PatternNode*>(e);
}

void *pattern_createGroup(void *l) {
	ElementList *list = reinterpret_cast<ElementList*>(l);
	return new GroupElement(list);
}

void *pattern_createList(bool group) {
	return new ElementList(group ? regex_groups++ : 0);
}

void pattern_addToList(void *l,void *e) {
	ElementList *list = reinterpret_cast<ElementList*>(l);
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

void *pattern_createChoice(void *l) {
	ElementList *list = reinterpret_cast<ElementList*>(l);
	return new ChoiceElement(list);
}

void *pattern_createCharClass(void *l,bool negate) {
	ElementList *list = reinterpret_cast<ElementList*>(l);
	return new CharClassElement(list,negate);
}

void *pattern_createCharClassElem(char begin,char end) {
	return new CharClassElement::Range(begin,end);
}

void *pattern_createSimpleCharClass(char begin,char end,bool negate) {
	ElementList *list = new ElementList(0);
	list->add(new CharClassElement::Range(begin,end));
	return new CharClassElement(list,negate);
}

void *pattern_createWS(bool negate) {
	ElementList *list = new ElementList(0);
	list->add(new CharClassElement::Range('\t','\r'));
	list->add(new CharClassElement::Range(' ',' '));
	return new CharClassElement(list,negate);
}

void *pattern_createWord(bool negate) {
	ElementList *list = new ElementList(0);
	list->add(new CharClassElement::Range('a','z'));
	list->add(new CharClassElement::Range('A','Z'));
	list->add(new CharClassElement::Range('0','9'));
	list->add(new CharClassElement::Range('_','_'));
	return new CharClassElement(list,negate);
}
