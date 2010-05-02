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
#include <exceptions/io.h>
#include <io/file.h>
#include <io/streams.h>
#include <io/ifilestream.h>
#include <util/cmdargs.h>

#define BUF_SIZE 512

static void printStream(sIStream *s);

static void usage(const char *name) {
	cerr->format(cerr,"Usage: %s [<file> ...]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	/* no exception here since we don't have required- or value-args */
	sCmdArgs *args = cmdargs_create(argc,argv,0);
	args->parse(args,"");
	if(args->isHelp)
		usage(argv[0]);

	if(argc < 2)
		printStream(cin);
	else {
		const char *arg;
		foreach(args->getFreeArgs(args),arg) {
			sIStream *s = NULL;
			sFile *f = NULL;
			TRY {
				f = file_get(arg);
				if(f->isDir(f))
					cerr->format(cerr,"'%s' is a directory!\n",arg);
				else {
					s = ifstream_open(arg,IO_READ);
					printStream(s);
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

static void printStream(sIStream *s) {
	s32 count;
	char buffer[BUF_SIZE];
	while((count = s->read(s,buffer,BUF_SIZE)) > 0)
		cout->write(cout,buffer,count);
}
