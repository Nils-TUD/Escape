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
#include <esc/vthrow.h>
#include <z/gzip.h>
#include <z/deflate.h>
#include <stdlib.h>
#include <stdio.h>

static void compress(FILE *f,const std::string &filename,bool tostdout,int compr) {
	FILE *out = stdout;
	if(!tostdout) {
		std::string name = filename + ".gz";
		out = fopen(name.c_str(),"w");
		if(!out) {
			throw esc::default_error("unable to open for writing");
			return;
		}
	}

	z::GZipHeader header(filename.c_str(),NULL,true);
	header.write(out);

	z::FileDeflateSource src(f);
	z::FileDeflateDrain drain(out);
	z::Deflate deflate;
	if(deflate.compress(&drain,&src,compr) != 0)
		throw esc::default_error("compressing failed");
	else {
		uint32_t crc32 = src.crc32();
		if(fwrite(&crc32,4,1,out) != 1)
			throw esc::default_error("unable to write CRC32");
		uint32_t orgsize = src.count();
		if(fwrite(&orgsize,4,1,out) != 1)
			throw esc::default_error("unable to write size of original file");
	}

	// TODO not reached if exception throws
	if(!tostdout)
		fclose(out);
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-c] <file>...\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	int tostdout = false;
	int compr = z::Deflate::FIXED;
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("c l=d",&tostdout,&compr);
		if(args.is_help() ||
			(compr != z::Deflate::NONE && compr != z::Deflate::FIXED && compr != z::Deflate::DYN))
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
			compress(f,**file,tostdout,compr);
		}
		catch(const std::exception &e) {
			std::cerr << **file << ": " << e.what() << "\n";
		}

		fclose(f);
	}
	return 0;
}
