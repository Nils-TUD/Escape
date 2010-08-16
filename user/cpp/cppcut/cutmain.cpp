/**
 * $Id: cutmain.c 647 2010-05-05 17:27:58Z nasmussen $
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

#include <esc/common.h>
#include <cmdargs.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <file.h>
#include <string.h>
#include <stdlib.h>

static void parseFields(const string& fields,s32 *first,s32 *last);
static void print(istream& s,const string& delim,int first,int last);

static void usage(const char *name) {
	cerr << "Usage: " << name << " -f <fields> [-d <delim>] [<file>]" << endl;
	cerr << "	-f: <fields> may be:" << endl;
	cerr << "		N		N'th field, counted from 1" << endl;
	cerr << "		N-		from N'th field, to end of line" << endl;
	cerr << "		N-M		from N'th to M'th (included) field" << endl;
	cerr << "		-M		from first to M'th (included) field" << endl;
	cerr << "	-d: use <delim> as delimiter instead of TAB" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	int first = 1,last = -1;
	string fields;
	string delim = "\t";

	cmdargs args(argc,argv,0);
	try {
		args.parse("f=s* d=s",&fields,&delim);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	parseFields(fields,&first,&last);

	cout.exceptions(ostream::failbit | ostream::badbit);
	const vector<string*>& fargs = args.get_free();
	if(fargs.empty())
		print(cin,delim,first,last);
	else {
		for(vector<string*>::const_iterator it = fargs.begin(); it != fargs.end(); ++it) {
			try {
				file f(**it);
				if(f.is_dir())
					cerr << "'" << **it << "' is a directory!" << endl;
				else {
					ifstream ifs((*it)->c_str());
					print(ifs,delim,first,last);
				}
			}
			catch(const io_exception& e) {
				cerr << "Unable to read file '" << **it << "': " << e.what() << endl;
			}
		}
	}
	return EXIT_SUCCESS;
}

static void parseFields(const string& fields,int *first,int *last) {
	istringstream s(fields);
	if(s.peek() == '-')
		*first = 1;
	else
		s >> *first;
	if(!s.eof() && s.get() == '-') {
		if(s.peek() != EOF)
			s >> *last;
		else
			*last = -1;
	}
	else
		*last = *first;
}

static void print(istream& s,const string& delim,int first,int last) {
	try {
		string line;
		while(getline(s,line)) {
			if(first == 0 && last == -1)
				cout << line << '\n';
			else {
				int i = 1;
				auto_array<char> cpy(new char[line.size() + 1]);
				std::copy(line.begin(),line.end(),cpy.get());
				cpy[line.size()] = '\0';
				char *tok = strtok(cpy.get(),delim.c_str());
				while(tok != NULL) {
					if(i >= first && (last == -1 || i <= last))
						cout << tok << ' ';
					tok = strtok(NULL,delim.c_str());
					i++;
				}
				cout << '\n';
			}
		}
	}
	catch(const ios_base::failure& e) {
		cerr << e.what() << endl;
	}
}
