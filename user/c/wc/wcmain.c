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
#include <esc/io/console.h>
#include <esc/io/file.h>
#include <esc/io/ifilestream.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define WC_BYTES	1
#define WC_WORDS	2
#define WC_LINES	4

static void countFile(sIStream *in);
static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-clw] [<file>...]\n",name);
	cerr->writef(cerr,"	-c: Print byte-count\n");
	cerr->writef(cerr,"	-l: Print line-count\n");
	cerr->writef(cerr,"	-w: Print word-count\n");
	exit(EXIT_FAILURE);
}

static u32 lines = 0;
static u32 bytes = 0;
static u32 words = 0;

int main(int argc,const char *argv[]) {
	u8 flags;
	sCmdArgs *args;

	TRY {
		bool flines = false,fwords = false,fbytes = false;
		args = cmdargs_create(argc,argv,0);
		args->parse(args,"w c l",&fwords,&fbytes,&flines);
		if(args->isHelp)
			usage(argv[0]);
		if(flines == false && fwords == false && fbytes == false)
			flags = WC_BYTES | WC_WORDS | WC_LINES;
		else
			flags = (flines ? WC_LINES : 0) | (fwords ? WC_WORDS : 0) | (fbytes ? WC_BYTES : 0);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	sIterator it = args->getFreeArgs(args);
	if(!it.hasNext(&it))
		countFile(cin);
	else {
		while(it.hasNext(&it)) {
			const char *arg = it.next(&it);
			sFile *f = NULL;
			sIStream *in = NULL;
			TRY {
				f = file_get(arg);
				if(f->isDir(f))
					cerr->writef(cerr,"'%s' is a directory!\n",arg);
				else {
					in = ifstream_open(arg,IO_READ);
					countFile(in);
				}
			}
			CATCH(IOException,e) {
				cerr->writef(cerr,"Unable to read '%s': %s\n",arg,e->toString(e));
			}
			ENDCATCH
			if(in)
				in->close(in);
			if(f)
				f->destroy(f);
		}
	}

	if(flags == WC_BYTES)
		cout->writef(cout,"%u\n",bytes);
	else if(flags == WC_WORDS)
		cout->writef(cout,"%u\n",words);
	else if(flags == WC_LINES)
		cout->writef(cout,"%u\n",lines);
	else {
		if(flags & WC_LINES)
			cout->writef(cout,"%7u",lines);
		if(flags & WC_WORDS)
			cout->writef(cout,"%7u",words);
		if(flags & WC_BYTES)
			cout->writef(cout,"%7u",bytes);
		cout->writef(cout,"\n");
	}
	return EXIT_SUCCESS;
}

static void countFile(sIStream *in) {
	u32 bufPos = 0;
	char c;
	while((c = in->readc(in)) != EOF) {
		if(isspace(c)) {
			if(bufPos > 0) {
				if(c == '\n')
					lines++;
				words++;
				bufPos = 0;
			}
		}
		else
			bufPos++;
		bytes++;
	}

	/* last word */
	if(bufPos > 0) {
		words++;
		lines++;
	}
}
