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
#include <esc/proc.h>
#include <io/streams.h>
#include <io/ifilestream.h>
#include <exceptions/io.h>
#include <exceptions/cmdargs.h>
#include <util/cmdargs.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN		512

static bool matches(const char *line,const char *pattern);
static void strtolower(char *s);
static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s <pattern> [<file>]\n",name);
	cerr->writef(cerr,"	<pattern> will be treated case-insensitive and is NOT a\n");
	cerr->writef(cerr,"	regular expression because we have no regexp-library yet ;)\n");
	exit(EXIT_FAILURE);
}

static char buffer[MAX_LINE_LEN];
static char lbuffer[MAX_LINE_LEN];

int main(int argc,const char *argv[]) {
	sIStream *in = cin;
	char *pattern = NULL;
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_MAX1_FREE);
		args->parse(args,"=s*",&pattern);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	TRY {
		s32 count;
		const char *inFile = args->getFirstFree(args);
		if(inFile)
			in = ifstream_open(inFile,IO_READ);

		strtolower(pattern);
		while(!in->eof(in)) {
			count = in->readline(in,buffer,MAX_LINE_LEN);
			if(count) {
				if(matches(buffer,pattern))
					cout->writef(cout,"%s\n",buffer);
			}
		}
	}
	CATCH(IOException,e) {
		error("Got IOException: %s",e->toString(e));
	}
	ENDCATCH

	in->close(in);
	args->destroy(args);
	return EXIT_SUCCESS;
}

static bool matches(const char *line,const char *pattern) {
	strcpy(lbuffer,line);
	strtolower(lbuffer);
	return strstr(lbuffer,pattern) != NULL;
}

static void strtolower(char *s) {
	while(*s) {
		if(isupper(*s))
			*s = tolower(*s);
		s++;
	}
}
