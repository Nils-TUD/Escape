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
#include <exceptions/cmdargs.h>
#include <exceptions/io.h>
#include <io/streams.h>
#include <io/istringstream.h>
#include <io/ifilestream.h>
#include <io/file.h>
#include <util/cmdargs.h>
#include <string.h>

#define MAX_LINE_LEN	255

static void printFields(char *line,const char *delim,s32 first,s32 last);
static void parseFields(const char *fields,s32 *first,s32 *last);

static void usage(const char *name) {
	cerr->format(cerr,"Usage: %s -f <fields> [-d <delim>] [<file>]\n",name);
	cerr->format(cerr,"	-f: <fields> may be:\n");
	cerr->format(cerr,"		N		N'th field, counted from 1\n");
	cerr->format(cerr,"		N-		from N'th field, to end of line\n");
	cerr->format(cerr,"		N-M		from N'th to M'th (included) field\n");
	cerr->format(cerr,"		-M		from first to M'th (included) field\n");
	cerr->format(cerr,"	-d: use <delim> as delimiter instead of TAB\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char **argv) {
	char line[MAX_LINE_LEN];
	s32 first = 1,last = -1;
	char *fields = NULL;
	char *delim = (char*)"\t";
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,0);
		args->parse(args,"f=s* d=s",&fields,&delim);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->format(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	parseFields(fields,&first,&last);

	sIterator it = args->getFreeArgs(args);
	if(!it.hasNext(&it)) {
		while(cin->readline(cin,line,sizeof(line)) > 0)
			printFields(line,delim,first,last);
	}
	else {
		while(it.hasNext(&it)) {
			sIStream *s = NULL;
			sFile *f = NULL;
			const char *arg = (const char*)it.next(&it);
			TRY {
				f = file_get(arg);
				if(f->isDir(f))
					cerr->format(cerr,"'%s' is a directory!\n",arg);
				else {
					s = ifstream_open(arg,IO_READ);
					while(s->readline(s,line,sizeof(line)) > 0)
						printFields(line,delim,first,last);
				}
			}
			CATCH(IOException,e) {
				cerr->format(cerr,"Unable to read file '%s': %s\n",arg,e->toString(e));
			}
			ENDCATCH
			if(f)
				f->destroy(f);
			if(s)
				s->close(s);
		}
	}

	args->destroy(args);
	return EXIT_SUCCESS;
}

static void printFields(char *line,const char *delim,s32 first,s32 last) {
	if(first == 0 && last == -1)
		cout->format(cout,"%s\n",line);
	else {
		s32 i = 1;
		char *tok = strtok(line,delim);
		while(tok != NULL) {
			if(i >= first && (last == -1 || i <= last))
				cout->format(cout,"%s ",tok);
			tok = strtok(NULL,delim);
			i++;
		}
		cout->writec(cout,'\n');
	}
}

static void parseFields(const char *fields,s32 *first,s32 *last) {
	sIStream *s = isstream_open(fields);
	if(s->getc(s) == '-')
		*first = 1;
	else
		s->format(s,"%d",first);
	if(!s->eof(s) && s->readc(s) == '-') {
		if(!s->eof(s))
			s->format(s,"%d",last);
		else
			*last = -1;
	}
	else
		*last = *first;
	s->close(s);
}
