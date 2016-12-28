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

#pragma once

#include <gui/graphics/rectangle.h>

class Preview {
public:
	static void create() {
		_inst = new Preview();
	}
	static Preview &get() {
		return *_inst;
	}

	/**
	 * Paints the given rect s a preview-rectangle with the given thickness in <shmem>. If <thickness>
	 * is 0, it is removed.
	 *
	 * @param shmem the screen memory
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the width
	 * @param height the height
	 * @param thickness the thickness of the lines
	 */
	void set(char *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height,gsize_t thickness);

	/**
	 * Ensures that the currently set preview-rectangle is painted again, if the given rectangle
	 * intersects with it.
	 *
	 * @param shmem the screen memory
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the width
	 * @param height the height
	 */
	void updateRect(char *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

private:
	void handleIntersec(char *shmem,const gui::Rectangle &curRec,
		const gui::Rectangle &intersec,size_t i,gsize_t xres,gsize_t yres);
	void clearRegion(char *shmem,gpos_t x,gpos_t y,gsize_t w,gsize_t h);
	void copyRegion(char *src,char *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1);

	gui::Rectangle _rect;
	gsize_t _thickness;
	char *_rectCopies[4];
	static Preview *_inst;
};
