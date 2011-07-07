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
		 * Destructor
		 */
		virtual ~ActionListener() {
		};

		/**
		 * Is called as soon as an action has been performed
		 *
		 * @param el the ui-element that received this event
		 */
		virtual void actionPerformed(UIElement& el) = 0;

	private:
		// no copying
		ActionListener(const ActionListener &l);
		ActionListener &operator=(const ActionListener &l);
	};
}

#endif /* ACTIONLISTENER_H_ */
