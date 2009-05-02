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
#include <esc/vector.h>
#include <esc/gui/common.h>

namespace esc {
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

		private:
			static Application *_inst;

		public:
			Application();
			virtual ~Application();

			int run();

		private:
			inline void AddWindow(Window *win) {
				_windows.add(win);
			};
			inline tFD getWinManagerFd() const {
				return _winFd;
			};
			inline tFD getVesaFd() const {
				return _vesaFd;
			};
			inline void *getVesaMem() const {
				return _vesaMem;
			};

		private:
			tFD _winFd;
			tFD _vesaFd;
			void *_vesaMem;
			Vector<Window*> _windows;
		};
	}
}


#endif /* APPLICATION_H_ */
