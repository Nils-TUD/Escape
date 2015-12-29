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

#include <esc/proto/screen.h>
#include <sys/common.h>
#include <sys/mman.h>
#include <algorithm>

namespace esc {

#define ABS(x)			((x) > 0 ? (x) : -(x))

std::vector<Screen::Mode> Screen::getModes() {
	std::vector<Mode> modes;
	ssize_t res = getModeCount();
	if(res < 0)
		VTHROWE("getModeCount()",res);
	if(res == 0)
		return modes;

	ValueResponse<size_t> r;
	_is << res << SendReceive(MSG_SCR_GETMODES) >> r;
	if(r.err < 0)
		VTHROWE("getModes()",r.err);

	Mode tmp[r.res];
	_is >> ReceiveData(tmp,sizeof(tmp));
	for(size_t i = 0; i < r.res; ++i)
		modes.push_back(tmp[i]);
	return modes;
}

Screen::Mode Screen::findTextModeIn(const std::vector<Mode> &modes,uint cols,uint rows) {
	/* search for the best matching mode */
	uint bestdiff = UINT_MAX;
	std::vector<Mode>::const_iterator bestmode = modes.end();
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		if(it->type & esc::Screen::MODE_TYPE_TUI) {
			uint pixdiff = ABS((int)(it->rows * it->cols) - (int)(cols * rows));
			if(pixdiff < bestdiff) {
				bestmode = it;
				bestdiff = pixdiff;
			}
		}
	}
	return *bestmode;
}

Screen::Mode Screen::findGraphicsModeIn(const std::vector<Mode> &modes,gsize_t width,gsize_t height,
		gcoldepth_t bpp) {
	/* search for the best matching mode */
	std::vector<Mode>::const_iterator best = modes.end();
	size_t pixdiff, bestpixdiff = ABS(320 * 200 - width * height);
	size_t depthdiff, bestdepthdiff = 8 >= bpp ? 8 - bpp : (bpp - 8) * 2;
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		if(it->type & esc::Screen::MODE_TYPE_GUI) {
			/* exact match? */
			if(it->width == width && it->height == height && it->bitsPerPixel == bpp) {
				best = it;
				break;
			}

			/* Otherwise, compare to the closest match so far, remember if best */
			pixdiff = ABS(it->width * it->height - width * height);
			if(it->bitsPerPixel >= bpp)
				depthdiff = it->bitsPerPixel - bpp;
			else
				depthdiff = (bpp - it->bitsPerPixel) * 2;
			if(bestpixdiff > pixdiff || (bestpixdiff == pixdiff && bestdepthdiff > depthdiff)) {
				best = it;
				bestpixdiff = pixdiff;
				bestdepthdiff = depthdiff;
			}
		}
	}
	return *best;
}

char *FrameBuffer::init(Screen::Mode &mode,const char *file,int type,int flags,uint perms) {
	char *res;
	/* open shm */
	int fd = shm_open(file,flags,perms);
	if(fd < 0)
		VTHROWE("shm_open(" << file << ")",fd);

	size_t size;
	if(type == esc::Screen::MODE_TYPE_TUI)
		size = mode.cols * (mode.rows + mode.tuiHeaderSize) * 2;
	else
		size = mode.width * (mode.height + mode.guiHeaderSize) * (mode.bitsPerPixel / 8);
	res = static_cast<char*>(mmap(NULL,size,0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0));
	close(fd);
	if(res == NULL) {
		if(flags & O_CREAT)
			shm_unlink(file);
		VTHROWE("mmap(" << size << ")",errno);
	}

	if(type == esc::Screen::MODE_TYPE_TUI)
		res += mode.tuiHeaderSize * mode.cols * 2;
	else
		res += mode.guiHeaderSize * mode.width * (mode.bitsPerPixel / 8);
	mode.type = type;
	return res;
}

FrameBuffer::~FrameBuffer() {
	if(_mode.type == esc::Screen::MODE_TYPE_TUI)
		_addr -= _mode.tuiHeaderSize * _mode.cols * 2;
	else
		_addr -= _mode.guiHeaderSize * _mode.width * (_mode.bitsPerPixel / 8);
	munmap(_addr);
	if(_created)
		shm_unlink(_filename.c_str());
}

}
