/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/endian.h>
#include <esc/cmdargs.h>
#include <z/gzip.h>
#include <z/deflate.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

static void uncompress(FILE *f,const std::string &filename,const z::GZipHeader &header,bool tostdout) {
	FILE *out = stdout;
	if(!tostdout) {
		std::string name;
		if(header.filename != NULL) {
			name = header.filename;
			if(name.find('/') != std::string::npos) {
				printe("%s: GZip contains invalid filename (%s)",filename.c_str(),name.c_str());
				return;
			}
		}
		else {
			if(filename.rfind(".gz") != filename.length() - 3) {
				printe("%s: invalid file name",filename.c_str());
				return;
			}

			name = filename.substr(0,filename.length() - 3);
		}

		out = fopen(name.c_str(),"w");
		if(!out) {
			printe("Unable to open '%s' for writing",filename.c_str());
			return;
		}
	}

	z::FileSource src(f);
	z::FileDrain drain(out);
	z::Deflate deflate;
	if(deflate.uncompress(&drain,&src) != 0)
		printe("%s: uncompressing failed",filename.c_str());
	else {
		uint32_t crc32;
		if(fread(&crc32,4,1,f) != 1)
			printe("%s: unable to read CRC32",filename.c_str());
		else if(le32tocpu(crc32) != drain.crc32())
			printe("%s: CRC32 is invalid",filename.c_str());
	}

	if(!tostdout)
		fclose(out);
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-c] [-i] <file>...\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	int showinfo = false;
	int tostdout = false;
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("c i",&tostdout,&showinfo);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		fprintf(stderr,"Invalid arguments: %s\n",e.what());
		usage(argv[0]);
	}

	for(auto file = args.get_free().begin(); file != args.get_free().end(); ++file) {
		FILE *f = fopen((*file)->c_str(),"r");
		if(f == NULL) {
			printe("Unable to open '%s' for reading",(*file)->c_str());
			continue;
		}

		try {
			z::GZipHeader header = z::GZipHeader::read(f);
			if(showinfo)
				std::cout << (*file)->c_str() << ":\n" << header << "\n";
			else
				uncompress(f,**file,header,tostdout);
		}
		catch(const std::exception &e) {
			std::cerr << **file << ": " << e.what() << "\n";
		}

		fclose(f);
	}
	return 0;
}
