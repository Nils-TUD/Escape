/**
 * $Id: kimain.c 661 2010-05-07 21:39:28Z nasmussen $
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
#include <iomanip>
#include <map>
#include <string>
#include <cmdargs.h>
#include <signal.h>
#include <stdlib.h>

static void usage(const char *name) {
	cerr << "Usage: " << name << " [-L] [-s <signal>] <pid>..." << endl;
	cerr << "	<signal> may be: SIGKILL, SIGTERM, SIGINT, KILL, TERM, INT" << endl;
	exit(EXIT_FAILURE);
}

static map<string,tSig> sigs;

int main(int argc,char *argv[]) {
	sigs["KILL"] = SIG_KILL;
	sigs["SIGKILL"] = SIG_KILL;
	sigs["TERM"] = SIG_TERM;
	sigs["SIGTERM"] = SIG_TERM;
	sigs["INT"] = SIG_INTRPT;
	sigs["SIGINT"] = SIG_INTRPT;

	tSig sig = SIG_KILL;
	string ssig;
	bool list = false;
	cmdargs args(argc,argv);
	try {
		args.parse("L s=s",&list,&ssig);
		if(args.is_help())
			usage(argv[0]);
		// translate signal-name to signal-number
		if(!ssig.empty()) {
			if(sigs.find(ssig) != sigs.end())
				sig = sigs[ssig];
			else {
				cerr << "Unknown signal '" << ssig << "'" << endl;
				usage(argv[0]);
			}
		}
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	/* print signals */
	if(list) {
		cout << setw(10) << "SIGKILL" << " - " << SIG_KILL << endl;
		cout << setw(10) << "SIGTERM" << " - " << SIG_TERM << endl;
		cout << setw(10) << "SIGINT" << " - " << SIG_INTRPT << endl;
	}
	else {
		const vector<string*>& fargs = args.get_free();
		for(vector<string*>::const_iterator it = fargs.begin(); it != fargs.end(); ++it) {
			tPid pid = atoi((*it)->c_str());
			if(pid > 0) {
				if(sendSignalTo(pid,sig,0) < 0)
					cerr << "Unable to send signal " << sig << " to " << pid << endl;
			}
			else if(**it != "0")
				cerr << "Unable to kill process with pid '" << **it << "'" << endl;
			else
				cerr << "You can't kill 'init'" << endl;
		}
	}
	return EXIT_SUCCESS;
}
