/**
 * $Id$
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

#ifndef PROGRESSBAR_H_
#define PROGRESSBAR_H_

#include <esc/common.h>
#include <gui/control.h>
#include <gui/color.h>
#include <string>

namespace gui {
	class ProgressBar : public Control {
	private:
		static const Color BGCOLOR;
		static const Color BARCOLOR;
		static const Color FGCOLOR;
		static const Color BORDER_COLOR;
		static const gsize_t PADDING	= 2;

	public:
		ProgressBar(const std::string &text)
			: Control(0,0,0,0), _position(0), _text(text) {
		};
		ProgressBar(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _position(0), _text(string()) {
		};
		ProgressBar(const std::string &text,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _position(0), _text(text) {
		};
		ProgressBar(const ProgressBar &b)
			: Control(b), _position(b._position), _text(b._text) {
		};
		virtual ~ProgressBar() {

		};
		ProgressBar &operator=(const ProgressBar &b);

		inline size_t getPosition() const {
			return _position;
		};
		inline void setPosition(size_t pos) {
			_position = MIN(100,pos);
			repaint();
		};
		inline std::string getText() const {
			return _text;
		};
		inline void setText(const std::string &text) {
			_text = text;
			repaint();
		};

		virtual gsize_t getPreferredWidth() const;
		virtual gsize_t getPreferredHeight() const;
		virtual void paint(Graphics &g);

	private:
		size_t _position;
		std::string _text;
	};
}

#endif /* PROGRESSBAR_H_ */
