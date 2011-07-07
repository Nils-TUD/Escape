/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <esc/common.h>
#include <esc/messages.h>
#include <vector>

namespace gui {
	class Window;
	class Graphics;

	class Application {
		friend class Window;
		friend class Graphics;

	public:
		static Application *getInstance() {
			// TODO multithreading..
			if(_inst == NULL)
				_inst = new Application();
			return _inst;
		};

	protected:
		static Application *_inst;

	public:
		inline const sVESAInfo *getVesaInfo() const {
			return &_vesaInfo;
		};
		inline gsize_t getScreenWidth() const {
			return _vesaInfo.width;
		};
		inline gsize_t getScreenHeight() const {
			return _vesaInfo.height;
		};
		inline uchar getColorDepth() const {
			return _vesaInfo.bitsPerPixel;
		};

		int run();

	protected:
		Application();
		virtual ~Application();

		virtual void doEvents();
		virtual void handleMessage(msgid_t mid,sMsg *msg);

	private:
		// prevent copying
		Application(const Application &a);
		Application &operator=(const Application &a);

		inline int getVesaFd() const {
			return _vesaFd;
		};
		inline void *getVesaMem() const {
			return _vesaMem;
		};
		void passToWindow(gwinid_t win,gpos_t x,gpos_t y,short movedX,short movedY,uchar buttons);
		void closePopups(gwinid_t id,gpos_t x,gpos_t y);
		void requestWinUpdate(gwinid_t id,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		void addWindow(Window *win);
		void removeWindow(Window *win);
		Window *getWindowById(gwinid_t id);
		void moveWindow(Window *win,bool finish);
		void resizeWindow(Window *win,bool finish);

	protected:
		int _winFd;
		sMsg _msg;
	private:
		uchar _mouseBtns;
		int _vesaFd;
		void *_vesaMem;
		sVESAInfo _vesaInfo;
		std::vector<Window*> _windows;
	};
}

#endif /* APPLICATION_H_ */
