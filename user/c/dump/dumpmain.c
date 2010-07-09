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
#include <esc/io/console.h>
#include <esc/io/ifilestream.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/cmdargs.h>
#include <esc/width.h>
#include <ctype.h>

#define BUF_SIZE		512
#define NPRINT_CHAR		'.'
#define OUT_FORMAT_OCT	'o'
#define OUT_FORMAT_DEC	'd'
#define OUT_FORMAT_HEX	'h'
#define MAX_BASE		16

static char ascii[MAX_BASE];
static u8 buffer[BUF_SIZE];

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-n <bytes>] [-f o|h|d] [<file>]\n",name);
	cerr->writef(cerr,"	-n <bytes>	: Read the first <bytes> bytes\n");
	cerr->writef(cerr,"	-f o|h|d	: The base to print the bytes in:\n");
	cerr->writef(cerr,"					o = octal\n");
	cerr->writef(cerr,"					h = hexadecimal\n");
	cerr->writef(cerr,"					d = decimal\n");
	exit(EXIT_FAILURE);
}

static void printAscii(u8 base,s32 pos) {
	s32 j;
	if(pos > 0) {
		while(pos % base != 0) {
			ascii[pos % base] = ' ';
			cout->writef(cout,"%*s ",getuwidth(0xFF,base)," ");
			pos++;
		}
		cout->writec(cout,'|');
		for(j = 0; j < base; j++)
			cout->writec(cout,ascii[j]);
		cout->writec(cout,'|');
	}
	cout->writec(cout,'\n');
}

int main(int argc,const char *argv[]) {
	sIStream *in = cin;
	u8 base = 16;
	char format = OUT_FORMAT_HEX;
	s32 count = -1;
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_MAX1_FREE);
		args->parse(args,"n=d f=c",&count,&format);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	/* TODO perhaps cmdargs should provide a possibility to restrict the values of an option */
	/* like 'arg=[ohd]' */
	if(format != OUT_FORMAT_DEC && format != OUT_FORMAT_HEX &&
			format != OUT_FORMAT_OCT)
		usage(argv[0]);

	switch(format) {
		case OUT_FORMAT_DEC:
			base = 10;
			break;
		case OUT_FORMAT_HEX:
			base = 16;
			break;
		case OUT_FORMAT_OCT:
			base = 8;
			break;
	}

	TRY {
		s32 i;
		const char *path = args->getFirstFree(args);
		if(path != NULL)
			in = ifstream_open(path,IO_READ);

		i = 0;
		while(count < 0 || count > 0) {
			s32 x,c;
			c = count >= 0 ? MIN(count,BUF_SIZE) : BUF_SIZE;
			c = in->read(in,buffer,c);
			if(c == 0)
				break;

			for(x = 0; x < c; x++, i++) {
				if(i % base == 0) {
					if(i > 0)
						printAscii(base,i);
					cout->writef(cout,"%08x: ",i);
				}

				if(isprint(buffer[x]) && buffer[x] < 0x80 && !isspace(buffer[x]))
					ascii[i % base] = buffer[x];
				else
					ascii[i % base] = NPRINT_CHAR;
				switch(format) {
					case OUT_FORMAT_DEC:
						cout->writef(cout,"%03d ",buffer[x]);
						break;
					case OUT_FORMAT_HEX:
						cout->writef(cout,"%02x ",buffer[x]);
						break;
					case OUT_FORMAT_OCT:
						cout->writef(cout,"%03o ",buffer[x]);
						break;
				}
			}

			if(count >= 0)
				count -= c;
		}

		printAscii(base,i);
	}
	CATCH(IOException,e) {
		error("Got an IOException: %s",e->toString(e));
	}
	ENDCATCH

	in->close(in);
	args->destroy(args);
	return EXIT_SUCCESS;
}
