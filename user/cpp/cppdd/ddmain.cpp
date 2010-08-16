/**
 * $Id: ddmain.c 651 2010-05-07 10:29:00Z nasmussen $
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
#include <rawfile.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>

static void usage(const char *name) {
	cerr << "Usage: " << name << " [if=<file>] [of=<file>] [bs=N] [count=N]" << endl;
	cerr << "	You can use the suffixes K, M and G to specify N" << endl;
	exit(EXIT_FAILURE);
}

static void interrupted(tSig sig,u32 data);

static bool run = true;

int main(int argc,char *argv[]) {
	u32 bs = 4096;
	u32 count = 0;
	u64 total = 0;
	string inFile;
	string outFile;
	rawfile in,out;

	cmdargs args(argc,argv,cmdargs::NO_DASHES | cmdargs::NO_FREE | cmdargs::REQ_EQ);
	try {
		args.parse("if=s of=s bs=k count=k",&inFile,&outFile,&bs,&count);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	if(setSigHandler(SIG_INTRPT,interrupted) < 0)
		error("Unable to set sig-handler for SIG_INTRPT");

	try {
		if(!inFile.empty())
			in.open(inFile,rawfile::READ);
		else
			in.use(STDIN_FILENO);
		if(!outFile.empty())
			out.open(outFile,rawfile::WRITE | rawfile::TRUNCATE);
		else
			out.use(STDOUT_FILENO);
	}
	catch(const io_exception& e) {
		error("Unable to open stream: %s",e.what());
	}

	try {
		rawfile::size_type res;
		u8 *buffer = new u8[bs];
		u64 limit = (u64)count * bs;
		while(run && (!count || total < limit)) {
			res = in.read(buffer,sizeof(u8),bs);
			out.write(buffer,sizeof(u8),res);
			total += res;
		}
		delete[] buffer;
	}
	catch(const io_exception& e) {
		error("Unable to transfer: %s",e.what());
	}

	cout.precision(3);
	cout << "Wrote " << total << " bytes in " << (float)(total / (double)bs) << " packages";
	cout << ", each " << bs << " bytes long" << endl;
	return EXIT_SUCCESS;
}

static void interrupted(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	run = false;
}
