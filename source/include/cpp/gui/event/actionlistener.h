/**
 * $Id$
 */

#pragma once

#include <esc/common.h>
#include <gui/uielement.h>

namespace gui {
	/**
	 * The abstract class to listen for actions (button-clicks, combobox-selections, ...)
	 */
	class ActionListener {
	public:
		ActionListener() {
		};
		virtual ~ActionListener() {
		};

		/**
		 * Is called as soon as an action has been performed
		 *
		 * @param el the ui-element that received this event
		 */
		virtual void actionPerformed(UIElement& el) = 0;
	};
}
