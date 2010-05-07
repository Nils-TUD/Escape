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
#include <esc/exceptions/io.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/io/console.h>
#include <esc/io/ifilestream.h>
#include <esc/util/cmdargs.h>
#include <esc/util/vector.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_INC		16
#define MAX_LINE_LEN	255

static int compareStrs(const void *a,const void *b);

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-r] [-i] [<file>]\n",name);
	cerr->writef(cerr,"	-r: reverse; i.e. descending instead of ascending\n");
	cerr->writef(cerr,"	-i: ignore case\n");
	exit(EXIT_FAILURE);
}

static bool igncase = false;
static bool reverse = false;

int main(int argc,const char *argv[]) {
	const char *filename = NULL;
	sIStream *in = cin;
	sVector *lines = NULL;
	sString *line;
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_MAX1_FREE);
		args->parse(args,"r i",&reverse,&igncase);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	TRY {
		/* a file? */
		filename = args->getFirstFree(args);
		if(filename)
			in = ifstream_open(filename,IO_READ);

		/* read lines */
		lines = vec_create(sizeof(sString*));
		while(!in->eof(in)) {
			char c;
			line = str_create();
			while(!in->eof(in) && (c = in->readc(in)) != '\n')
				str_appendc(line,c);
			vec_add(lines,&line);
		}
	}
	CATCH(IOException,e) {
		error("Got an IO-Exception: %s",e->toString(e));
	}
	ENDCATCH

	/* sort */
	vec_sortCustom(lines,compareStrs);

	/* print */
	vforeach(lines,line) {
		cout->writeln(cout,line->str);
		str_destroy(line);
	}
	vec_destroy(lines,false);
	in->close(in);
	return EXIT_SUCCESS;
}

static int compareStrs(const void *a,const void *b) {
	sString *s1 = *(sString**)a;
	sString *s2 = *(sString**)b;
	s32 res;
	if(igncase)
		res = strcasecmp(s1->str,s2->str);
	else
		res = strcmp(s1->str,s2->str);
	return reverse ? -res : res;
}
