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

#include <img/bitmapimage.h>
#include <img/image.h>
#include <img/pngimage.h>
#include <sys/common.h>
#include <stdio.h>

namespace img {

Image *Image::loadImage(const std::shared_ptr<Painter> &painter,const std::string& path) {
	char header[MAX(PNGImage::SIG_LEN,BitmapImage::SIG_LEN)];
	FILE *f = fopen(path.c_str(),"r");
	if(!f)
		throw img_load_error(path + ": Unable to open");
	fread(header,1,sizeof(header),f);
	fclose(f);
	// check header-type
	if(memcmp(header,BitmapImage::SIG,BitmapImage::SIG_LEN) == 0)
		return new BitmapImage(painter,path);
	if(memcmp(header,PNGImage::SIG,PNGImage::SIG_LEN) == 0)
		return new PNGImage(painter,path);
	// unknown image-type
	throw img_load_error(path + ": Unknown image-type (header " + header + ")");
}

}
