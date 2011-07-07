/**
 * $Id$
 */

#ifndef EVENTLIST_H_
#define EVENTLIST_H_

#include <esc/common.h>
#include <gui/uielement.h>
#include <vector>

namespace gui {
	template<class T>
	class EventList {
	public:
		EventList() : _listener(std::vector<T*>()) {
		};
		EventList(const EventList& l) : _listener(l._listener) {
		};
		~EventList() {
		};
		EventList& operator=(const EventList& l) {
			if(this == &l)
				return *this;
			_listener = l._listener;
			return *this;
		}

		inline void addListener(T *l) {
			_listener.push_back(l);
		}
		inline void removeListener(T *l) {
			_listener.erase_first(l);
		}

	protected:
		std::vector<T*> _listener;
	};
}

#endif /* EVENTLIST_H_ */
