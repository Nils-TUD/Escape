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

#include <esc/stream/fstream.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <z/gzip.h>
#include <z/inflate.h>
#include <stdlib.h>

using namespace esc;

static int showinfo = false;
static int tostdout = false;
static int keep = false;

static void uncompress(IStream &is,const std::string &filename) {
	z::GZipHeader header = z::GZipHeader::read(is);
	if(showinfo)
		sout << filename.c_str() << ":\n" << header << "\n";
	else {
		OStream *out = &sout;
		if(!tostdout) {
			std::string name;
			if(header.filename != NULL) {
				name = header.filename;
				if(name.find('/') != std::string::npos) {
					errmsg(filename << ": GZip contains invalid filename (" << name << ")");
					return;
				}
			}
			else {
				if(filename.rfind(".gz") != filename.length() - 3) {
					errmsg(filename << ": invalid file name");
					return;
				}

				name = filename.substr(0,filename.length() - 3);
			}

			out = new FStream(name.c_str(),"w");
			if(!*out) {
				errmsg("Unable to open '" << name << "' for writing");
				delete out;
				return;
			}
		}

		z::StreamInflateSource src(is);
		z::StreamInflateDrain drain(*out);
		z::Inflate inflate;
		if(inflate.uncompress(&drain,&src) != 0)
			errmsg(filename << ": uncompressing failed");
		else {
			uint32_t crc32 = (uint8_t)src.get() << 0;
			crc32 |= (uint8_t)src.get() << 8;
			crc32 |= (uint8_t)src.get() << 16;
			crc32 |= (uint8_t)src.get() << 24;
			if(le32tocpu(crc32) != drain.crc32())
				errmsg(filename << ": CRC32 is invalid");
		}

		if(!tostdout) {
			delete out;
			if(!keep && unlink(filename.c_str()) < 0)
				errmsg("Unable to unlink '" << filename << "'");
		}
	}
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-c] [-i] [-k] [<file>...]\n";
	serr << "  -c: write to stdout\n";
	serr << "  -i: don't uncompress given files, but show information about them\n";
	serr << "  -k: keep the original files, don't delete them\n";
	serr << "  If no file is given or <file> is '-', stdin is uncompressed to stdout.\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	cmdargs args(argc,argv,0);
	try {
		args.parse("c i k",&tostdout,&showinfo,&keep);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	if(args.get_free().size() == 0 || (args.get_free().size() == 1 && *(args.get_free()[0]) == "-")) {
		tostdout = true;
		try {
			uncompress(sin,"stdin");
		}
		catch(const std::exception &e) {
			errmsg("stdin: " << e.what());
		}
	}
	else {
		for(auto file = args.get_free().begin(); file != args.get_free().end(); ++file) {
			FStream f((*file)->c_str(),"r");
			if(!f) {
				errmsg("Unable to open '" << **file << "' for reading");
				continue;
			}

			try {
				uncompress(f,**file);
			}
			catch(const std::exception &e) {
				errmsg(**file << ": " << e.what());
			}
		}
	}
	return 0;
}
