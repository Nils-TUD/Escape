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
#include <img/bitmapimage.h>
#include <sys/endian.h>

#include "bmp.h"

bool BMPImageInfo::print(esc::OStream &os,const std::string &file) {
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

	os << "Bitmap image (BMP)\n";
	os << "  Size           : " << ihead.width << " x " << ihead.height << "\n";
	os << "  Bit depth      : " << ihead.bitCount << "\n";
	os << "  Compression    : ";
	os << (ihead.compression < ARRAY_SIZE(compr) ? compr[ihead.compression] : "??") << "\n";
	os << "  Red mask       : #" << esc::fmt(ihead.redmask,"0x",8) << "\n";
	os << "  Green mask     : #" << esc::fmt(ihead.greenmask,"0x",8) << "\n";
	os << "  Blue mask      : #" << esc::fmt(ihead.bluemask,"0x",8) << "\n";
	os << "  Alpha mask     : #" << esc::fmt(ihead.alphamask,"0x",8) << "\n";
	os << "  Red mask       : #" << esc::fmt(ihead.redmask,"0x",8) << "\n";
	return true;
}
