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

// TODO it would be nice to be reentrant
static const char *regex_err = NULL;
const char *regex_patstr = NULL;
void *regex_result = NULL;
size_t regex_groups = 0;
int regex_flags = 0;

/* Called by yyparse on error.  */
extern "C" void yyerror(char const *s) {
	regex_err = s;
}

namespace esc {

esc::OStream &operator<<(esc::OStream &os,const Regex::Pattern &p) {
	p._root->print(os,0);
	return os;
}

Regex::Pattern Regex::compile(const std::string &regex) {
	regex_err = NULL;
	regex_result = NULL;
	regex_patstr = regex.c_str();
	regex_groups = 0;
	regex_flags = 0;

	int res = yyparse();
	yylex_destroy();
	if(res != 0 || regex_err) {
		pattern_destroy(regex_result);
		throw std::runtime_error(regex_err ? regex_err : "Unknown error");
	}

	Regex::Element *root = reinterpret_cast<Regex::Element*>(regex_result);
	return Regex::Pattern(root,regex_flags);
}

Regex::Result Regex::test(const Pattern &p,const std::string &str,size_t pos) {
	Input in(str,pos);
	Regex::Result res(regex_groups);
	if(p.root()->match(&res,in))
		res.setSuccess(true);
	return res;
}

Regex::Result Regex::search(const Pattern &p,const std::string &str) {
	if(p.flags() & REGEX_FLAG_BEGIN)
		return test(p,str,0);
	for(size_t i = 0; i < str.length(); ++i) {
		Regex::Result res = test(p,str,i);
		if(res.matched()) {
			if((~p.flags() & REGEX_FLAG_END) || (i + res.get(0).length() == str.length()))
				return res;
		}
	}
	return Regex::Result();
}

bool Regex::matches(const Pattern &p,const std::string &str) {
	Input in(str);
	return p.root()->match(NULL,in) && in.peek() == '\0';
}

std::string Regex::replace(const Pattern &p,const std::string &str,const std::string &repl) {
	Result res = search(p,str);
	if(res.matched()) {
		OStringStream os;
		for(size_t i = 0; i < repl.length(); ++i) {
			if(repl[i] == '$') {
				char *end = NULL;
				int group = strtoul(repl.c_str() + i + 1,&end,10);
				if(end != repl.c_str() + i + 1) {
					os << res.get(group);
					i += end - (repl.c_str() + i + 1);
					continue;
				}
			}
			os << repl[i];
		}
		return os.str();
	}
	return str;
}


}
