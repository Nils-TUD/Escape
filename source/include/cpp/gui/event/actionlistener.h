/**
 * $Id$
 */

#ifndef ACTIONLISTENER_H_
#define ACTIONLISTENER_H_

#include <esc/common.h>
#include <gui/uielement.h>

namespace gui {
	/**
	 * The abstract class to listen for actions (button-clicks, combobox-selections, ...)
	 */
	class ActionListener {
	public:
		/**
		 * Empty constructor
		 */
		ActionListener() {
		};
		/**
		 * Copy-constructor
		 */
		ActionListener(const ActionListener &) {
		};
		/**
		 * Destructor
		 */
		virtual ~ActionListener() {
		};
		/**
		 * Assignment-operator
		 */
		ActionListener &operator=(const ActionListener &) {
			return *this;
		};

		/**
		 * Is called as soon as an action has been performed
		 *
		 * @param el the ui-element that received this event
		 */
		virtual void actionPerformed(UIElement& el) = 0;
	};
}

#endif /* ACTIONLISTENER_H_ */
