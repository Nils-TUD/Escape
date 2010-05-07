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
#include <signal.h>
#include <esc/proc.h>
#include <esc/mem/heap.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/io/console.h>
#include <esc/io/ifilestream.h>
#include <esc/io/ofilestream.h>
#include <esc/util/cmdargs.h>

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [if=<file>] [of=<file>] [bs=N] [count=N]\n",name);
	cerr->writef(cerr,"	You can use the suffixes K, M and G to specify N\n");
	exit(EXIT_FAILURE);
}

static void interrupted(tSig sig,u32 data);

static bool run = true;

int main(int argc,const char *argv[]) {
	u32 bs = 4096;
	u32 count = 0;
	u64 total = 0;
	char *inFile = NULL;
	char *outFile = NULL;
	sIStream *in = cin;
	sOStream *out = cout;
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_NO_DASHES | CA_NO_FREE | CA_REQ_EQ);
		args->parse(args,"if=s of=s bs=k count=k",&inFile,&outFile,&bs,&count);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	if(setSigHandler(SIG_INTRPT,interrupted) < 0)
		error("Unable to set sig-handler for SIG_INTRPT");

	TRY {
		if(inFile)
			in = ifstream_open(inFile,IO_READ);
		if(outFile)
			out = ofstream_open(outFile,IO_WRITE | IO_TRUNCATE | IO_CREATE);
	}
	CATCH(IOException,e) {
		error("Unable to open stream: %s",e->toString(e));
	}
	ENDCATCH

	TRY {
		s32 res;
		u8 *buffer = heap_alloc(bs);
		u64 limit = (u64)count * bs;
		while(run && (!count || total < limit)) {
			if((res = in->read(in,buffer,bs)) <= 0)
				break;
			out->write(out,buffer,res);
			total += res;
		}
		heap_free(buffer);
	}
	CATCH(IOException,e) {
		error("Unable to transfer: %s",e->toString(e));
	}
	ENDCATCH

	cout->writef(cout,"Wrote %Lu bytes in %.3f packages, each %u bytes long\n",
			total,(float)(total / (double)bs),bs);

	out->close(out);
	in->close(in);
	args->destroy(args);
	return EXIT_SUCCESS;
}

static void interrupted(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	run = false;
}
