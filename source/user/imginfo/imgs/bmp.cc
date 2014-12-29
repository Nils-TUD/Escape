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

#include <sys/endian.h>
#include <img/bitmapimage.h>
#include <esc/rawfile.h>
#include <iostream>
#include <iomanip>

#include "bmp.h"

bool BMPImageInfo::print(const std::string &file) {
	esc::rawfile f(file,esc::rawfile::READ);

	// check header
	img::BitmapImage::FileHeader fhead;
	f.read(&fhead,1,sizeof(fhead));
	if(memcmp(fhead.type,img::BitmapImage::SIG,img::BitmapImage::SIG_LEN) != 0)
		return false;

	img::BitmapImage::InfoHeader ihead;
	f.read(&ihead,1,sizeof(ihead));

	static const char *compr[] = {
		"RGB","RLE8","RLE4","Bitfields"
	};

	std::cout << "Bitmap image (BMP)\n";
	std::cout << "  Size           : " << ihead.width << " x " << ihead.height << "\n";
	std::cout << "  Bit depth      : " << ihead.bitCount << "\n";
	std::cout << "  Compression    : ";
	std::cout << (ihead.compression < ARRAY_SIZE(compr) ? compr[ihead.compression] : "??") << "\n";
	std::cout << std::hex << std::setfill('0');
	std::cout << "  Red mask       : #" << std::setw(8) << ihead.redmask << "\n";
	std::cout << "  Green mask     : #" << std::setw(8) << ihead.greenmask << "\n";
	std::cout << "  Blue mask      : #" << std::setw(8) << ihead.bluemask << "\n";
	std::cout << "  Alpha mask     : #" << std::setw(8) << ihead.alphamask << "\n";
	std::cout << "  Red mask       : #" << std::setw(8) << ihead.redmask << "\n";
	std::cout << std::dec << std::setfill(' ');
	return true;
}
