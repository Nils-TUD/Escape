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

#include <cmdargs.h>
#include <ctype.h>

namespace std {
	cmdargs_error::cmdargs_error(const string& arg)
		: _msg(arg) {
	}
	cmdargs_error::~cmdargs_error() throw () {
	}
	const char* cmdargs_error::what() const throw () {
		return _msg.c_str();
	}

	cmdargs::~cmdargs() {
		for(auto it = _args.begin(); it != _args.end(); ++it)
			delete *it;
	}

	void cmdargs::parse(const char *fmt,...) {
		const char *helps[] = {"-h","--help","-?"};

		// copy arguments to vector of strings
		for(int i = 0; i < _argc; i++) {
			// check if its a help-request
			if(i > 0 && !_ishelp) {
				for(unsigned int j = 0; j < ARRAY_SIZE(helps); j++) {
					if(strcmp(_argv[i],helps[j]) == 0) {
						_ishelp = true;
						break;
					}
				}
			}
			_args.push_back(new string(_argv[i]));
		}
		_free = _args;
		// we don't want to have the prog-name in the free args
		_free.erase(_free.begin());
		if(_ishelp)
			return;

		// loop through the argument-specification and set the variables to the given arguments
		va_list ap;
		va_start(ap,fmt);
		while(*fmt) {
			string name;
			bool required = false;
			bool hasVal = false;
			char c,type = '\0';
			// first read the name
			while((c = *fmt) && c != '=' && c != ' ' && c != '*') {
				name += c;
				fmt++;
			}

			// has it a value?
			if(*fmt == '=') {
				hasVal = true;
				fmt++;
				/* read val-type and if its required */
				while(*fmt && *fmt != ' ') {
					switch(*fmt) {
						case 's':
						case 'd':
						case 'i':
						case 'u':
						case 'x':
						case 'X':
						case 'k':
						case 'c':
							type = *fmt;
							break;
						case '*':
							required = true;
							break;
					}
					fmt++;
				}
			}

			/* find the argument and set it */
			vector<string*>::iterator pos;
			string arg = find(name,hasVal,pos);
			if(required && pos == _args.end())
				throw cmdargs_error(string("Required argument '") + name + "' is missing");
			setval(arg,pos,hasVal,type,va_arg(ap,void*));

			/* to next */
			while(*fmt && *fmt++ != ' ');
		}
		va_end(ap);

		/* check free args */
		if((_flags & NO_FREE) && _free.size() > 0)
			throw cmdargs_error("Free arguments are not allowed");
		if((_flags & MAX1_FREE) && _free.size() > 1)
			throw cmdargs_error("Max. 1 free argument is allowed");
	}

	string cmdargs::find(const string& name,bool hasVal,vector<string*>::iterator& pos) {
		/* empty? */
		if(name.empty()) {
			// use the first free arg
			if(_free.size() == 0)
				throw cmdargs_error("Required argument is missing");
			pos = _args.begin();
			return string(*(_free[0]));
		}
		for(auto it = _args.begin(); it != _args.end(); ++it) {
			string *arg = *it;
			unsigned int index;
			if(name.size() == 1) {
				index = (_flags & NO_DASHES) ? 1 : 2;
				if(!(_flags & NO_DASHES) && (arg->size() == 0 || arg->at(0) != '-'))
					continue;
				// multiple flags may be passed with one argument; don't search in args with
				// because this can't be flag-arguments.
				if(!hasVal && arg->find('=') == string::npos &&
						arg->find(name[0],index - 1) != string::npos) {
					pos = it;
					return string(1,name[0]);
				}
				// otherwise the first char has to match and has to be the last, too
				if(arg->compare(index - 1,1,name) != 0)
					continue;
			}
			else {
				index = (_flags & NO_DASHES) ? name.size() : name.size() + 2;
				if(_flags & NO_DASHES) {
					if(arg->compare(0,name.size(),name) != 0)
						continue;
				}
				else {
					if(arg->compare(0,2,"--") != 0 || arg->compare(2,name.size(),name) != 0)
						continue;
				}
			}
			// check if either a '=' follows or if its the end of the argument
			if(!(!(_flags & NO_EQ) && arg->size() > index && arg->at(index) == '=') &&
				!(!(_flags & REQ_EQ) && index == arg->size()))
				continue;
			// return substring
			pos = it;
			unsigned int offset = index - name.size();
			if(hasVal) {
				string::size_type i = arg->find('=');
				if(i == string::npos) {
					if((_flags & REQ_EQ) || pos == _args.end() - 1)
						throw cmdargs_error("Please use '=' to specify values");
					return string(**(it + 1));
				}
				else
					return string(arg->begin() + i + 1,arg->end());
			}
			return string(arg->begin() + offset,arg->begin() + offset + name.size());
		}
		pos = _args.end();
		return string();
	}

	void cmdargs::setval(const string& arg,vector<string*>::iterator pos,bool hasVal,
			char type,void *ptr) {
		if(hasVal) {
			if(pos == _args.end())
				return;

			_free.erase_first(*pos);
			if((*pos)->find('=') == string::npos)
				_free.erase_first(*(pos + 1));
			else if(_flags & NO_EQ)
				throw cmdargs_error("Please use no '=' to specify values");

			switch(type) {
				case 's': {
					string *str = (string*)ptr;
					*str = arg;
				}
				break;

				case 'c': {
					char *c = (char*)ptr;
					*c = arg[0];
				}
				break;

				case 'd':
				case 'i': {
					int *n = (int*)ptr;
					*n = strtol(arg.c_str(),nullptr,10);
				}
				break;

				case 'u': {
					uint *n = (uint*)ptr;
					*n = strtoul(arg.c_str(),nullptr,10);
				}
				break;

				case 'x':
				case 'X': {
					unsigned int *u = (unsigned int*)ptr;
					*u = strtoul(arg.c_str(),nullptr,16);
				}
				break;

				case 'k': {
					unsigned int *k = (unsigned int*)ptr;
					*k = readk(arg);
				}
				break;
			}
		}
		else {
			bool *b = (bool*)ptr;
			*b = !arg.empty();
			if(*b)
				_free.erase_first(*pos);
		}
	}

	unsigned int cmdargs::readk(const string& arg) {
		auto it = arg.begin();
		unsigned int val = 0;
		for(; isdigit(*it) && it != arg.end(); ++it)
			val = val * 10 + (*it - '0');
		switch(*it) {
			case 'K':
			case 'k':
				val *= 1024;
				break;
			case 'M':
			case 'm':
				val *= 1024 * 1024;
				break;
			case 'G':
			case 'g':
				val *= 1024 * 1024 * 1024;
				break;
		}
		return val;
	}
}
