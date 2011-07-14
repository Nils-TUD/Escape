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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <esc/common.h>
#include <esc/messages.h>
#include <gui/graphicsbuffer.h>
#include <gui/windowlistener.h>
#include <gui/theme.h>
#include <exception>
#include <vector>

namespace gui {
	class Window;
	class Color;

	/**
	 * The exception that will be thrown if something goes wrong in Application. For example,
	 * if receiving a message from the window-manager failed.
	 */
	class app_error : public std::exception {
	public:
		app_error(const std::string& str) throw ()
			: exception(), _str(str) {
		}
		~app_error() throw () {
		}
		virtual const char *what() const throw () {
			return _str.c_str();
		}
	private:
		std::string _str;
	};

	/**
	 * The class Application is a singleton, because there is only one instance per application. It
	 * manages the communication with vesa and the window-manager, knows all windows and runs the
	 * event-loop.
	 */
	class Application {
		friend class Window;
		friend class GraphicsBuffer;
		friend class Color;

	public:
		/**
		 * @return the instance of the class
		 */
		static Application *getInstance() {
			// TODO multithreading..
			if(_inst == NULL)
				_inst = new Application();
			return _inst;
		};

	private:
		static Application *_inst;

	public:
		/**
		 * @return the width of the screen
		 */
		inline gsize_t getScreenWidth() const {
			return _vesaInfo.width;
		};
		/**
		 * @return the height of the screen
		 */
		inline gsize_t getScreenHeight() const {
			return _vesaInfo.height;
		};
		/**
		 * @return the color-depth
		 */
		inline uchar getColorDepth() const {
			return _vesaInfo.bitsPerPixel;
		};

		/**
		 * @param id the window-id
		 * @return the window with given id or NULL
		 */
		Window *getWindowById(gwinid_t id);

		/**
		 * @return the default-theme
		 */
		inline const Theme *getDefaultTheme() {
			return &_defTheme;
		};

		/**
		 * Adds the given window-listener to the list
		 *
		 * @param l the listener
		 */
		void addWindowListener(WindowListener *l,bool global);
		/**
		 * Removes the given window-listener listener from the list
		 *
		 * @param l the listener
		 */
		void removeWindowListener(WindowListener *l);

		/**
		 * Requests that this window should be the active one
		 */
		void requestActiveWindow(gwinid_t wid);

		/**
		 * Starts the message-loop
		 */
		int run();

		/**
		 * Stops the message-loop
		 */
		void exit();

	private:
		/**
		 * Constructor. Protected because its a singleton
		 */
		Application();
		/**
		 * Destructor. Closes the connection to vesa and the window-manager
		 */
		virtual ~Application();
		/**
		 * Handles the given message
		 *
		 * @param mid the message-id
		 * @param msg the received message
		 */
		virtual void handleMessage(msgid_t mid,sMsg *msg);

	private:
		// prevent copying
		Application(const Application &a);
		Application &operator=(const Application &a);

		void notifyCreate(gwinid_t id,const std::string& title);
		void notifyActive(gwinid_t id);
		void notifyDestroy(gwinid_t id);

		/**
		 * @return the information received from vesa
		 */
		inline const sVESAInfo *getVesaInfo() const {
			return &_vesaInfo;
		};
		/**
		 * @return the file-descriptor for vesa
		 */
		inline int getVesaFd() const {
			return _vesaFd;
		};
		/**
		 * @return the shared-memory for the screen
		 */
		inline void *getScreenMem() const {
			return _vesaMem;
		};
		/**
		 * Requests an update for the window with given id and the given rectangle.
		 * Ensures that the update does only affect the given window
		 *
		 * @param id the window-id
		 * @param x the x-position (global)
		 * @param y the y-position (global)
		 * @param width the width
		 * @param height the height
		 */
		void requestWinUpdate(gwinid_t id,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		/**
		 * Adds the given window to the window-list. Will announce the window at the window-manager
		 *
		 * @param win the window
		 */
		void addWindow(Window *win);
		/**
		 * Removes the given window from the window-list. Will remove it from the window-manager
		 *
		 * @param win the window
		 */
		void removeWindow(Window *win);
		/**
		 * Performs a move-operation with the given window. It will use getMoveX() and getMoveY()
		 * as destination. If finish is true, the window will really be moved, otherwise the preview
		 * rectangle will be displayed/moved.
		 *
		 * @param win the window
		 * @param finish whether to finish this operation
		 */
		void moveWindow(Window *win,bool finish);
		/**
		 * Performs a resize-operation with the given window. It will use getMoveX(), getMoveY(),
		 * getResizeWidth() and getResizeHeight() as destination. This way, a move and resize may
		 * be performed at once (e.g. when resizing on the left side). If finish is true, the
		 * window will really be resized, otherwise the preview rectangle will be displayed/resized.
		 *
		 * @param win the window
		 * @param finish whether to finish this operation
		 */
		void resizeWindow(Window *win,bool finish);

		// only used in Application itself
		void passToWindow(gwinid_t win,gpos_t x,gpos_t y,short movedX,short movedY,
				short movedZ,uchar buttons);
		void closePopups(gwinid_t id,gpos_t x,gpos_t y);

	private:
		int _winFd;
		sMsg _msg;
		bool _run;
		uchar _mouseBtns;
		int _vesaFd;
		void *_vesaMem;
		sVESAInfo _vesaInfo;
		std::vector<Window*> _windows;
		std::vector<std::pair<WindowListener*,bool> > _wlisten;
		bool _listening;
		Theme _defTheme;
	};
}

#endif /* APPLICATION_H_ */
