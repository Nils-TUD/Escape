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

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <z/deflate.h>
#include <z/gzip.h>
#include <stdlib.h>

using namespace esc;

static int compr = z::Deflate::FIXED;
static int tostdout = false;
static int keep = false;

static void compress(IStream &is,const std::string &filename) {
	OStream *out = &sout;
	if(!tostdout) {
		std::string name = filename + ".gz";
		out = new FStream(name.c_str(),"w");
		if(!*out) {
			errmsg(name << ": unable to open for writing");
			delete out;
			return;
		}
	}

	z::GZipHeader header(&is == &sin ? NULL : filename.c_str(),NULL,true);
	header.write(*out);

	z::StreamDeflateSource src(is);
	z::StreamDeflateDrain drain(*out);
	z::Deflate deflate;
	if(deflate.compress(&drain,&src,compr) != 0)
		errmsg(filename << ": compressing failed");
	else {
		uint32_t crc32 = src.crc32();
		if(out->write(&crc32,4) != 4)
			errmsg(filename << ": unable to write CRC32");
		else {
			uint32_t orgsize = src.count();
			if(out->write(&orgsize,4) != 4)
				errmsg(filename << ": unable to write size of original file");
		}
	}

	if(!tostdout) {
		delete out;
		if(!keep && unlink(filename.c_str()) < 0)
			errmsg("Unable to unlink '" << filename << "'");
	}
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-c] [-k] [<file>...]\n";
	serr << "  -c: write to stdout\n";
	serr << "  -k: keep the original files, don't delete them\n";
	serr << "  If no file is given or <file> is '-', stdin is compressed to stdout.\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("c l=d k",&tostdout,&compr,&keep);
		if(args.is_help() ||
			(compr != z::Deflate::NONE && compr != z::Deflate::FIXED && compr != z::Deflate::DYN))
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	if(args.get_free().size() == 0 || (args.get_free().size() == 1 && *(args.get_free()[0]) == "-")) {
		tostdout = true;
		compress(sin,"stdin");
	}
	else {
		for(auto file = args.get_free().begin(); file != args.get_free().end(); ++file) {
			FStream f((*file)->c_str(),"r");
			if(!f) {
				errmsg("Unable to open '" << **file << "' for reading");
				continue;
			}

			compress(f,**file);
		}
	}
	return 0;
}
