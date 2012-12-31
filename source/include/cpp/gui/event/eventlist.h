/**
 * $Id$
 */

#pragma once

#include <esc/common.h>
#include <gui/uielement.h>
#include <vector>

namespace gui {
	/**
	 * This class-template exists for convenience to allow classes to provide listeners and fire
	 * events. The listener-class can be anything, because the method that fires an event and calls
	 * all listener-methods has to be written by the class that uses this template.
	 *
	 * @param T the listener-class
	 */
	template<class T>
	class EventList {
	public:
		/**
		 * Constructor
		 */
		EventList() : _listener() {
		};

		/**
		 * Adds the given listener to the list
		 *
		 * @param l the listener
		 */
		inline void addListener(T *l) {
			_listener.push_back(l);
		}
		/**
		 * Removes the given listener from the list
		 *
		 * @param l the listener
		 */
		inline void removeListener(T *l) {
			_listener.erase_first(l);
		}

	protected:
		std::vector<T*> _listener;
	};
}
