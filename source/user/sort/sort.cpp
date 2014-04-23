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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmdargs.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

static bool compareStrs(const string& a,const string& b);
static void usage(const char *name) {
	cerr << "Usage: " << name << " [-r] [-i] [<file>]" << '\n';
	cerr << "	-r: reverse; i.e. descending instead of ascending" << '\n';
	cerr << "	-i: ignore case" << '\n';
	exit(EXIT_FAILURE);
}

static int figncase = 0;
static int freverse = 0;

int main(int argc,char *argv[]) {
	istream *in = &cin;

	cmdargs args(argc,argv,cmdargs::MAX1_FREE);
	try {
		args.parse("r i",&freverse,&figncase);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	// use arg?
	if(!args.get_free().empty()) {
		in = new ifstream((args.get_free()[0])->c_str());
		if(!in->good())
			error("Open failed");
	}

	// read lines
	string line;
	vector<string> lines;
	while(getline(*in,line))
		lines.push_back(line);

	// close if it has been opened
	if(in != &cin)
		delete in;

	// sort and print
	sort(lines.begin(),lines.end(),compareStrs);
	for(auto it = lines.begin(); it != lines.end(); ++it)
		cout << *it << '\n';
	return EXIT_SUCCESS;
}

static bool compareStrs(const string& a,const string& b) {
	bool res;
	if(figncase)
		res = strcasecmp(a.c_str(),b.c_str()) < 0;
	else
		res = a < b;
	return freverse ? !res : res;
}
