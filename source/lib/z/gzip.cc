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

#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <z/crc32.h>
#include <z/gzip.h>
#include <iomanip>
#include <time.h>

namespace z {

/* based on http://tools.ietf.org/html/rfc1952 */

static void printFlags(std::ostream &os,uint8_t flags,const char **names,size_t count) {
	if(flags == 0)
		os << "-";
	else {
		for(size_t i = 0; i < count; ++i) {
			if(flags & (1 << i))
				os << names[i] << " ";
		}
	}
}

static void readStr(FILE *f,char *dst,size_t max) {
	char c;
	size_t i;
	for(i = 0; i < max - 1 && (c = fgetc(f)) != '\0'; ++i)
		dst[i] = c;
	dst[i] = '\0';
}

GZipHeader::GZipHeader(const char *_filename,const char *_comment,bool hchksum)
	: id1(0x1f), id2(0x8b), method(MDEFLATE), flags(), mtime(time(NULL)), xflags(), os(3),
	  filename(), comment() {
	if(hchksum)
		flags |= FHCRC;

	if(_filename) {
		filename = new char[strlen(_filename) + 1];
		strcpy(filename,_filename);
		flags |= FNAME;
	}

	if(_comment) {
		comment = new char[strlen(_comment) + 1];
		strcpy(comment,_comment);
		flags |= FCOMMENT;
	}
}

GZipHeader GZipHeader::read(FILE *f) {
	CRC32 c;
	GZipHeader h;
	ulong checksum = 0;

	fread(&h,1,10,f);
	checksum = c.get(&h,10);
	h.mtime = le32tocpu(h.mtime);

	if(!h.isGZip())
		throw esc::default_error("No GZip file");
	if(h.method != MDEFLATE)
		throw esc::default_error("Unknown compression method");

	if(h.flags & FEXTRA) {
		uint16_t len;
		fread(&len,sizeof(len),1,f);
		char *buf = new char[len];
		fread(buf,1,len,f);
		checksum = c.update(checksum,buf,len);
		delete[] buf;
	}

	if(h.flags & FNAME) {
		h.filename = new char[MAX_NAME_LEN];
		readStr(f,h.filename,MAX_NAME_LEN);
		checksum = c.update(checksum,h.filename,strlen(h.filename) + 1);
	}

	if(h.flags & FCOMMENT) {
		h.comment = new char[MAX_COMMENT_LEN];
		readStr(f,h.comment,MAX_NAME_LEN);
		checksum = c.update(checksum,h.comment,strlen(h.comment) + 1);
	}

	if(h.flags & FHCRC) {
		uint16_t crc16 = 0;
		fread(&crc16,sizeof(crc16),1,f);
		crc16 = le16tocpu(crc16);

		if(crc16 != (checksum & 0x0000ffff))
			throw esc::default_error("GZip has invalid checksum");
	}
	return h;
}

void GZipHeader::write(FILE *f) {
	assert(isGZip() && method == MDEFLATE && (flags & ~(FNAME | FCOMMENT | FHCRC)) == 0 && xflags == 0);
	mtime = cputole32(mtime);
	try {
		if(fwrite(this,1,10,f) != 10)
			throw esc::default_error("Unable to write header");

		if(flags & FNAME) {
			assert(filename != NULL);
			size_t len = strlen(filename) + 1;
			if(fwrite(filename,1,len,f) != len)
				throw esc::default_error("Unable to write filename");
		}

		if(flags & FCOMMENT) {
			assert(comment != NULL);
			size_t len = strlen(comment) + 1;
			if(fwrite(comment,1,len,f) != len)
				throw esc::default_error("Unable to write comment");
		}

		if(flags & FHCRC) {
			CRC32 crc;
			ulong chksum = crc.get(this,10);
			if(flags & FNAME)
				chksum = crc.update(chksum,filename,strlen(filename) + 1);
			if(flags & FCOMMENT)
				chksum = crc.update(chksum,comment,strlen(comment) + 1);
			uint16_t hcrc = cputole16(chksum & 0xFFFF);
			if(fwrite(&hcrc,1,2,f) != 2)
				throw esc::default_error("Unable to write header CRC");
		}
		mtime = le32tocpu(mtime);
	}
	catch(...) {
		mtime = le32tocpu(mtime);
		throw;
	}
}

std::ostream &operator<<(std::ostream &os,const GZipHeader &h) {
	static const char *fnames[] = {
		"text","hcrc","extra","name","comment"
	};
	static const char *xfnames[] = {
		"??","slow","fast"
	};
	static const char *osnames[] = {
		"FAT filesystem (MS-DOS, OS/2, NT/Win32)","Amiga","VMS (or OpenVMS)","Unix","VM/CMS",
		"Atari TOS","HPFS filesystem (OS/2, NT)","Macintosh","Z-System","CP/M",
		"TOPS-20","NTFS filesystem (NT)","QDOS","Acorn RISCOS"
	};

	os << "id:       " << std::hex << std::setw(2) << h.id1 << ".";
	os << std::setw(2) << h.id2 << std::dec << "\n";

	os << "method:   " << (h.method == GZipHeader::MDEFLATE ? "deflate" : "unknown") << "\n";

	os << "flags:    ";
	printFlags(os,h.flags,fnames,ARRAY_SIZE(fnames));
	os << "\n";

	os << "modified: ";

	char buf[32];
	time_t mtime = le32tocpu(h.mtime);
	struct tm *t = gmtime(&mtime);
	strftime(buf,sizeof(buf),"%a, %d.%m.%Y %H:%M:%S %Z",t);
	os << buf << "\n";

	os << "xflags:   ";
	printFlags(os,h.xflags,xfnames,ARRAY_SIZE(xfnames));
	os << "\n";

	os << "os:       " << (h.os < ARRAY_SIZE(osnames) ? osnames[h.os] : "unknown") << "\n";
	os << "filename: " << (h.filename ? h.filename : "") << "\n";
	os << "comment:  " << (h.comment ? h.comment : "") << "\n";
	return os;
}

}
