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

#include <esc/rawfile.h>
#include <img/pngimage.h>
#include <sys/endian.h>
#include <iomanip>
#include <iostream>

#include "png.h"

bool PNGImageInfo::print(const std::string &file) {
	esc::rawfile f(file,esc::rawfile::READ);

	// check header
	uint8_t header[img::PNGImage::SIG_LEN];
	f.read(header,1,sizeof(header));
	if(memcmp(header,img::PNGImage::SIG,img::PNGImage::SIG_LEN) != 0)
		return false;

	std::cout << "Portable Network Graphics image (PNG)\n";
	while(1) {
		img::PNGImage::Chunk head;
		f.read(&head,sizeof(head),1);

		head.length = be32tocpu(head.length);
		std::cout << "  " << head.type[0] << head.type[1] << head.type[2] << head.type[3] << ": ";
		std::cout << head.length << " bytes\n";

		if(memcmp(head.type,"IEND",4) == 0)
			break;
		if(memcmp(head.type,"IHDR",4) == 0) {
			img::PNGImage::IHDR ihdr;
			f.read(&ihdr,sizeof(ihdr),1);
			ihdr.width = be32tocpu(ihdr.width);
			ihdr.height = be32tocpu(ihdr.height);
			std::cout << ihdr;
			f.seek(4,SEEK_CUR);
		}
		else if(memcmp(head.type,"tEXt",4) == 0) {
			std::unique_ptr<char[]> buf(new char[head.length + 1]);
			f.read(buf.get(),1,head.length);
			buf[head.length] = '\0';
			size_t keywordLen = strlen(buf.get());
			std::cout << "    " << buf.get() << ": " << (buf.get() + keywordLen + 1) << "\n";
			f.seek(4,SEEK_CUR);
		}
		else if(memcmp(head.type,"tIME",4) == 0) {
			img::PNGImage::tIME tm;
			f.read(&tm,sizeof(tm),1);
			tm.year = be16tocpu(tm.year);
			std::cout << "    " << tm << "\n";
			f.seek(4 + sizeof(tm) - head.length,SEEK_CUR);
		}
		else if(memcmp(head.type,"bKGD",4) == 0) {
			std::unique_ptr<uint8_t[]> buf(new uint8_t[head.length]);
			f.read(buf.get(),1,head.length);
			std::cout << "    " << std::setfill('0') << std::hex;
			for(size_t i = 0; i < head.length; ++i) {
				if(i > 0 && i % 16 == 0)
					std::cout << "\n";
				std::cout << std::setw(2) << buf[i] << ' ';
			}
			std::cout << std::setfill(' ') << std::dec << "\n";
			f.seek(4,SEEK_CUR);
		}
		else
			f.seek(head.length + 4,SEEK_CUR);
	}
	return true;
}
