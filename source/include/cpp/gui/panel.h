/**
 * $Id$
 */

#ifndef PANEL_H_
#define PANEL_H_

#include <esc/common.h>
#include <gui/control.h>
#include <gui/event.h>
#include <vector>

namespace gui {
	class Window;

	/**
	 * A panel is a container for control-elements
	 */
	class Panel : public Control {
		friend class Window;
		friend class Control;

	public:
		/**
		 * Constructor
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		Panel(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _focus(NULL), _bgColor(Color(0x88,0x88,0x88)),
			  _controls(vector<Control*>()) {
		};
		/**
		 * Clones the given panel
		 *
		 * @param p the panel
		 */
		Panel(const Panel &p)
			: Control(p), _focus(p._focus), _bgColor(p._bgColor), _controls(p._controls) {
			// TODO clone the controls!
		};
		/**
		 * Destructor
		 */
		virtual ~Panel() {
		};
		/**
		 * Assignment-operator
		 *
		 * @param p the panel
		 */
		Panel &operator=(const Panel &p) {
			if(this == &p)
				return *this;
			_focus = p._focus;
			_bgColor = p._bgColor;
			// TODO clone the controls!
			_controls = p._controls;
			return *this;
		};

		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMousePressed(const MouseEvent &e);

		/**
		 * @return the background-color
		 */
		inline Color getBGColor() const {
			return _bgColor;
		};
		/**
		 * Sets the background-color
		 *
		 * @param bg the background-color
		 */
		inline void setBGColor(Color bg) {
			_bgColor = bg;
		};

		virtual void moveTo(gpos_t x,gpos_t y);

		/**
		 * Paints the panel
		 *
		 * @param g the graphics-object
		 */
		virtual void paint(Graphics &g);
		/**
		 * Adds the given control to this panel
		 *
		 * @param c the control
		 */
		void add(Control &c);

	private:
		/**
		 * @return the control that has the focus (not a panel!) or NULL if no one
		 */
		virtual const Control *getFocus() const {
			if(_focus)
				return _focus->getFocus();
			return NULL;
		}
		virtual Control *getFocus() {
			if(_focus)
				return _focus->getFocus();
			return NULL;
		};

		void setFocus(Control *c) {
			_focus = c;
		};

	protected:
		Control *_focus;
		Color _bgColor;
		vector<Control*> _controls;
	};

	ostream &operator<<(ostream &s,const Panel &p);
}

#endif /* PANEL_H_ */
