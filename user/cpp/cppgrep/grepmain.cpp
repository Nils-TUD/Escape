/**
 * $Id$
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
#include <iostream>
#include <fstream>
#include <cmdargs.h>
#include <stdlib.h>
#include <ctype.h>

using namespace std;

static void readlines(istream &is,const string& pattern);
static bool matches(const string& line,const string& pattern);

static void usage(const char *name) {
	cerr << "Usage: " << name << " <pattern> [<file>]" << endl;
	cerr << "	<pattern> will be treated case-insensitive and is NOT a" << endl;
	cerr << "	regular expression because we have no regexp-library yet ;)" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	string pattern;
	cmdargs args(argc,argv,cmdargs::MAX1_FREE);

	try {
		args.parse("=s*",&pattern);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	try {
		transform(pattern.begin(),pattern.end(),pattern.begin(),::tolower);

		const vector<string*>& fargs = args.get_free();
		if(!fargs.empty()) {
			ifstream ifs(fargs[0]->c_str());
			readlines(ifs,pattern);
		}
		else
			readlines(cin,pattern);
	}
	catch(const exception& e) {
		cerr << e.what() << endl;
	}
	return EXIT_SUCCESS;
}

static void readlines(istream &is,const string& pattern) {
	string line;
	while(getline(is,line)) {
		if(matches(line,pattern))
			cout << line << '\n';
	}
}

static bool matches(const string& line,const string& pattern) {
	string cpy;
	cpy.reserve(line.size());
	transform(line.begin(),line.end(),cpy.begin(),::tolower);
	return cpy.find(pattern) != string::npos;
}
