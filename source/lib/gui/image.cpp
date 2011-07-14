/**
 * $Id: button.cpp 965 2011-07-07 10:56:45Z nasmussen $
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <gui/image.h>
#include <gui/bitmapimage.h>
#include <string.h>
#include <stdio.h>

namespace gui {
	Image *Image::loadImage(const string& path) {
		char header[3];
		FILE *f = fopen(path.c_str(),"r");
		if(!f)
			throw img_load_error(path + ": Unable to open");
		header[0] = fgetc(f);
		header[1] = fgetc(f);
		header[2] = '\0';
		fclose(f);
		// check header-type
		if(header[0] == 'B' && header[1] == 'M')
			return new BitmapImage(path);
		// unknown image-type
		throw img_load_error(path + ": Unknown image-type (header " + header + ")");
	}
}
