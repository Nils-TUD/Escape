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
#include <esc/regex/elements.h>
#include <esc/stream/std.h>

#include "pattern.h"

static const char *regex_err = NULL;
// TODO improve that
const char *regex_patstr = NULL;
void *regex_result = NULL;

/* Called by yyparse on error.  */
extern "C" void yyerror(char const *s) {
	regex_err = s;
}

namespace esc {

esc::OStream &operator<<(esc::OStream &os,const Regex::Pattern &p) {
	p._root->print(os,0);
	return os;
}

Regex::Pattern Regex::compile(const std::string &pattern) {
	regex_err = NULL;
	regex_result = NULL;
	regex_patstr = pattern.c_str();

	int res = yyparse();
	yylex_destroy();
	if(res != 0 || regex_err) {
		pattern_destroy(regex_result);
		throw std::runtime_error(regex_err ? regex_err : "Unknown error");
	}

	Regex::Element *root = reinterpret_cast<Regex::Element*>(regex_result);
	return Regex::Pattern(root);
}

bool Regex::matches(Pattern &p,const std::string &str) {
	Input in(str);
	return p.root()->match(in) && in.peek() == '\0';
}

}
