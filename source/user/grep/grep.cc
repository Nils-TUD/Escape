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

#include <esc/cmdargs.h>
#include <esc/regex/regex.h>
#include <esc/stream/std.h>
#include <esc/stream/fstream.h>

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " [-i] <pattern> [<file>]\n";
	serr << "    -i: match case insensitive\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	FStream *in = &sin;
	bool ci = false;

	// parse params
	cmdargs args(argc,argv,0);
	try {
		args.parse("i",&ci);
		if(args.is_help() || args.get_free().size() < 1)
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		serr << "Invalid arguments: " << e.what() << "\n";
		usage(argv[0]);
	}

	std::string regex = *args.get_free()[0];
	if(args.get_free().size() > 1) {
		in = new FStream(args.get_free()[1]->c_str(),"r");
		if(!in)
			error("Unable to open '%s'",args.get_free()[1]->c_str());
	}

	uint flags = Regex::NONE;
	if(ci)
		flags |= Regex::CASE_INSENSITIVE;
	Regex::Pattern pattern = Regex::compile(regex);

	std::string line;
	while(sout.good() && !in->eof()) {
		in->getline(line);
		if(Regex::search(pattern,line,flags).matched())
			sout << line << '\n';
	}
	if(in->error())
		error("Read failed");
	if(sout.error())
		error("Write failed");

	if(args.get_free().size() > 1)
		delete in;
	return EXIT_SUCCESS;
}
