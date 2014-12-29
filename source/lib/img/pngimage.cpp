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
#include <img/pngimage.h>
#include <z/inflate.h>
#include <esc/rawfile.h>
#include <iostream>
#include <iomanip>

namespace img {

const size_t PNGImage::SIG_LEN = 8;
const uint8_t PNGImage::SIG[] = {
	137,80,78,71,13,10,26,10
};

void PNGImage::paint(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	size_t p = y * ((_header.width * _bpp) + 1);
	gpos_t yend = y + height;
	for(gpos_t cy = y; cy < yend; cy++) {
		// skip over filter type byte
		p++;
		// skip over line-start
		p += x * _bpp;

		gpos_t xend = x + width;
		for(gpos_t cx = x; cx < xend; cx++) {
			uint32_t col;
			switch(_bpp) {
				// RGBA
				case 4:
					col = _pixels[p++] << 16;
					col |= _pixels[p++] << 8;
					col |= _pixels[p++] << 0;
					col |= (0xFF - _pixels[p++]) << 24;
					break;
				// RGB
				case 3:
					col = _pixels[p++] << 16;
					col |= _pixels[p++] << 8;
					col |= _pixels[p++] << 0;
					break;
				// grayscale
				case 1:
					col = _pixels[p++];
					col |= (col << 8) | (col << 16);
					break;
			}
			_painter->paintPixel(cx,cy,col);
		}

		// skip over end of line
		p += (_header.width - width) * _bpp;
	}
}

uint8_t PNGImage::left(size_t off,size_t x) {
	if(x == 0)
		return 0;
	return _pixels[off - _bpp];
}

uint8_t PNGImage::above(size_t off,size_t y) {
	if(y == 0)
		return 0;
	return _pixels[off - ((_header.width * _bpp) + 1)];
}

uint8_t PNGImage::leftabove(size_t off,size_t x,size_t y) {
	if(y == 0 || x == 0)
		return 0;
	return _pixels[off - ((_header.width * _bpp) + 1 + _bpp)];
}

uint8_t PNGImage::paethPredictor(uint8_t a,uint8_t b,uint8_t c) {
	int p = a + b - c;						// initial estimate
	int pa = p > a ? p - a : -(p - a);		// distances
	int pb = p > b ? p - b : -(p - b);
	int pc = p > c ? p - c : -(p - c);
	// return nearest of a,b,c
	// breaking ties in order a,b,c
	if(pa <= pb && pa <= pc)
		return a;
	if(pb <= pc)
		return b;
	return c;
}

void PNGImage::applyFilters() {
	size_t p = 0;
	for(size_t y = 0; y < _header.height; ++y) {
		uint8_t ft = _pixels[p++];
		if(ft != FT_NONE) {
			for(size_t x = 0; x < _header.width; ++x) {
				switch(ft) {
					case FT_SUB:
						for(int i = 0; i < _bpp; ++i)
							_pixels[p + i] += left(p + i,x);
						break;
					case FT_UP:
						for(int i = 0; i < _bpp; ++i)
							_pixels[p + i] += above(p + i,y);
						break;
					case FT_AVERAGE:
						for(int i = 0; i < _bpp; ++i)
							_pixels[p + i] += (left(p + i,x) + above(p + i,y)) / 2;
						break;
					case FT_PAETH:
						for(int i = 0; i < _bpp; ++i) {
							_pixels[p + i] += paethPredictor(
								left(p + i,x),above(p + i,y),leftabove(p + i,x,y));
						}
						break;
				}
				p += _bpp;
			}
		}
		else
			p += _header.width * _bpp;
	}
	fflush(stdout);
}

void PNGImage::load(const std::string &filename) {
	esc::rawfile f;

	// read and check signature
	uint8_t header[SIG_LEN];
	try {
		f.open(filename,esc::rawfile::READ);
		f.read(header,1,sizeof(header));
	}
	catch(esc::default_error &e) {
		throw img_load_error(filename + ": unable to open or read header: " + e.what());
	}
	if(memcmp(header,SIG,SIG_LEN) != 0)
		throw img_load_error(filename + ": invalid PNG signature");

	// read header and count size of data (it's a split zlib archive)
	size_t total = 0;
	std::unique_ptr<z::MemInflateDrain> drain;
	while(1) {
		Chunk head;
		if(f.read(&head,sizeof(head),1) != 1)
			throw img_load_error(filename + ": unable to read chunk header");
		head.length = be32tocpu(head.length);

		if(memcmp(head.type,"IEND",4) == 0)
			break;

		if(memcmp(head.type,"IHDR",4) == 0) {
			f.read(&_header,sizeof(_header),1);
			_header.width = be32tocpu(_header.width);
			_header.height = be32tocpu(_header.height);

			// check our current limitations
			if(_header.bitDepth != 8)
				throw img_load_error(filename + ": only bit-depth 8 is supported");
			if(_header.comprMethod != CM_DEFLATE)
				throw img_load_error(filename + ": only compression method 'deflate' is supported");
			if(_header.filterMethod != FM_ADAPTIVE)
				throw img_load_error(filename + ": only filter method 'adaptive' is supported");
			if(_header.interlaceMethod != IM_NONE)
				throw img_load_error(filename + ": only interlace method 'none' is supported");

			// determine bytes per pixel
			if(_header.colorType == CT_RGB_ALPHA)
				_bpp = 4;
			else if(_header.colorType == CT_RGB)
				_bpp = 3;
			else if(_header.colorType == CT_GRAYSCALE)
				_bpp = 1;
			else
				throw img_load_error(filename + ": color-type is not supported");

			_pixels = new uint8_t[(_header.width + 1) * (_header.height * _bpp)];
			drain.reset(new z::MemInflateDrain(_pixels,(_header.width + 1) * (_header.height * _bpp)));

			// skip CRC32
			f.seek(4,SEEK_CUR);
		}
		else if(memcmp(head.type,"IDAT",4) == 0) {
			if(_pixels == NULL)
				throw img_load_error(filename + ": invalid chunk order");

			// the first is preceeded by 2 bytes of compression method and flags
			// all others only contain data
			total += total == 0 ? head.length - 2 : head.length;

			// skip CRC32
			f.seek(head.length + 4,SEEK_CUR);
		}
		else
			f.seek(head.length + 4,SEEK_CUR);
	}

	// start over
	f.seek(SIG_LEN,SEEK_SET);

	// read chunks into buffer
	size_t pos = 0;
	std::unique_ptr<uint8_t[]> buffer(new uint8_t[total]);
	while(1) {
		Chunk head;
		if(f.read(&head,sizeof(head),1) != 1)
			throw img_load_error(filename + ": unable to read chunk header");
		head.length = be32tocpu(head.length);

		if(memcmp(head.type,"IEND",4) == 0)
			break;

		if(memcmp(head.type,"IDAT",4) == 0) {
			size_t len = head.length;
			// skip over header
			if(pos == 0) {
				f.seek(2,SEEK_CUR);
				len -= 2;
			}

			// read data into buffer
			if(f.read(buffer.get() + pos,1,len) != len)
				throw img_load_error(filename + ": unable to load IDAT chunk");
			pos += len;

			// skip CRC32
			f.seek(4,SEEK_CUR);
		}
		else
			f.seek(head.length + 4,SEEK_CUR);
	}

	// now uncompress all data chunks
	z::MemInflateSource source(buffer.get(),total);
	z::Inflate inflate;
	if(inflate.uncompress(&*drain,&source) != 0)
		throw img_load_error(filename + ": uncompress failed");
}

std::ostream &operator<<(std::ostream &os,const PNGImage::IHDR &h) {
	static const char *coltypes[] = {
		"grayscale","??","rgb","palette","grayscale-alpha","??","rgb-alpha"
	};
	static const char *intertypes[] = {
		"none","adam7"
	};
	os << "    Size           : " << h.width << " x " << h.height << "\n";
	os << "    Bit depth      : " << h.bitDepth << "\n";
	os << "    Color type     : ";
	os << (h.colorType < ARRAY_SIZE(coltypes) ? coltypes[h.colorType] : "??");
	os << "\n";
	os << "    Compr. Method  : " << (h.comprMethod == 0 ? "deflate" : "??") << "\n";
	os << "    Filter Method  : " << (h.filterMethod == 0 ? "adaptive" : "??") << "\n";
	os << "    Interl. Method : ";
	os << (h.interlaceMethod < ARRAY_SIZE(intertypes) ? intertypes[h.interlaceMethod] : "??");
	os << "\n";
	return os;
}

std::ostream &operator<<(std::ostream &os,const PNGImage::tIME &tm) {
	os << std::setfill('0');
	os << std::setw(2) << tm.month << "/" << std::setw(2) << tm.day << "/" << tm.year << " ";
	os << std::setw(2) << tm.hour << ":" << std::setw(2) << tm.minute;
	os << ":" << std::setw(2) << tm.second;
	os << std::setfill(' ');
	return os;
}

}
