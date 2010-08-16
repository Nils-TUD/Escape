/**
 * $Id: dumpmain.c 696 2010-07-09 13:22:02Z nasmussen $
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
#include <esc/width.h>
#include <iostream>
#include <rawfile.h>
#include <cmdargs.h>
#include <ctype.h>
#include <stdlib.h>

#define BUF_SIZE		512
#define NPRINT_CHAR		'.'
#define OUT_FORMAT_OCT	'o'
#define OUT_FORMAT_DEC	'd'
#define OUT_FORMAT_HEX	'h'
#define MAX_BASE		16

static char ascii[MAX_BASE];
static u8 buffer[BUF_SIZE];

static void usage(const char *name) {
	cerr << "Usage: " << name << " [-n <bytes>] [-f o|h|d] [<file>]" << endl;
	cerr << "	-n <bytes>	: Read the first <bytes> bytes" << endl;
	cerr << "	-f o|h|d	: The base to print the bytes in:" << endl;
	cerr << "					o = octal" << endl;
	cerr << "					h = hexadecimal" << endl;
	cerr << "					d = decimal" << endl;
	exit(EXIT_FAILURE);
}

static void printAscii(u8 base,s32 pos) {
	s32 j;
	if(pos > 0) {
		while(pos % base != 0) {
			ascii[pos % base] = ' ';
			cout.format("%*s ",getuwidth(0xFF,base)," ");
			pos++;
		}
		cout << '|';
		for(j = 0; j < base; j++)
			cout << ascii[j];
		cout << '|';
	}
	cout << '\n';
}

int main(int argc,char *argv[]) {
	rawfile in;
	u8 base = 16;
	char format = OUT_FORMAT_HEX;
	s32 count = -1;

	cmdargs args(argc,argv,cmdargs::MAX1_FREE);
	try {
		args.parse("n=d f=c",&count,&format);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

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

	try {
		s32 i;
		const vector<string*>& fargs = args.get_free();
		if(!fargs.empty())
			in.open(*(fargs[0]),rawfile::READ);
		else
			in.use(STDIN_FILENO);

		i = 0;
		while(count < 0 || count > 0) {
			s32 x,c;
			c = count >= 0 ? MIN(count,BUF_SIZE) : BUF_SIZE;
			c = in.read(buffer,sizeof(u8),c);
			if(c == 0)
				break;

			for(x = 0; x < c; x++, i++) {
				if(i % base == 0) {
					if(i > 0)
						printAscii(base,i);
					cout.format("%08x: ",i);
				}

				if(isprint(buffer[x]) && buffer[x] < 0x80 && !isspace(buffer[x]))
					ascii[i % base] = buffer[x];
				else
					ascii[i % base] = NPRINT_CHAR;
				switch(format) {
					case OUT_FORMAT_DEC:
						cout.format("%03d ",buffer[x]);
						break;
					case OUT_FORMAT_HEX:
						cout.format("%02x ",buffer[x]);
						break;
					case OUT_FORMAT_OCT:
						cout.format("%03o ",buffer[x]);
						break;
				}
			}

			if(count >= 0)
				count -= c;
		}

		printAscii(base,i);
	}
	catch(const io_exception& e) {
		error("Got an io-exception: %s",e.what());
	}
	return EXIT_SUCCESS;
}
