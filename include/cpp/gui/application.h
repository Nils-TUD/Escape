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
#include <gui/common.h>
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
		inline tSize getScreenWidth() const {
			return _vesaInfo.width;
		};
		inline tSize getScreenHeight() const {
			return _vesaInfo.height;
		};
		inline u8 getColorDepth() const {
			return _vesaInfo.bitsPerPixel;
		};

		int run();

	protected:
		Application();
		virtual ~Application();

		virtual void doEvents();
		virtual void handleMessage(tMsgId mid,const sMsg *msg);

	private:
		// prevent copying
		Application(const Application &a);
		Application &operator=(const Application &a);

		inline tFD getVesaFd() const {
			return _vesaFd;
		};
		inline void *getVesaMem() const {
			return _vesaMem;
		};
		void passToWindow(tWinId win,tCoord x,tCoord y,s16 movedX,s16 movedY,u8 buttons);
		void closePopups(tWinId id,tCoord x,tCoord y);
		void requestWinUpdate(tWinId id,tCoord x,tCoord y,tSize width,tSize height);
		void addWindow(Window *win);
		void removeWindow(Window *win);
		Window *getWindowById(tWinId id);
		void moveWindow(Window *win);
		void resizeWindow(Window *win);

	protected:
		tFD _winFd;
		sMsg _msg;
	private:
		u8 _mouseBtns;
		tFD _vesaFd;
		void *_vesaMem;
		sVESAInfo _vesaInfo;
		std::vector<Window*> _windows;
	};
}

#endif /* APPLICATION_H_ */
