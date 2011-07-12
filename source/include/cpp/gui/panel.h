/**
 * $Id$
 */

#ifndef PANEL_H_
#define PANEL_H_

#include <esc/common.h>
#include <gui/control.h>
#include <gui/event.h>
#include <gui/layout.h>
#include <esc/rect.h>
#include <vector>

namespace gui {
	class Window;
	class ScrollPane;

	/**
	 * A panel is a container for control-elements
	 */
	class Panel : public Control {
		friend class Window;
		friend class Control;
		friend class ScrollPane;

	private:
		static const Color DEF_BGCOLOR;

	public:
		Panel(Layout *layout)
			: Control(0,0,0,0), _focus(NULL), _bgColor(DEF_BGCOLOR),
			  _controls(vector<Control*>()), _layout(layout) {
		};
		/**
		 * Constructor
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		Panel(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _focus(NULL), _bgColor(DEF_BGCOLOR),
			  _controls(vector<Control*>()), _layout(NULL) {
		};
		/**
		 * Clones the given panel
		 *
		 * @param p the panel
		 */
		Panel(const Panel &p)
			: Control(p), _focus(p._focus), _bgColor(p._bgColor),
			  _controls(p._controls), _layout(p._layout) {
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
			_layout = p._layout;
			return *this;
		};

		virtual gsize_t getPreferredWidth() const {
			return _layout ? _layout->getPreferredWidth() : 0;
		};
		virtual gsize_t getPreferredHeight() const {
			return _layout ? _layout->getPreferredHeight() : 0;
		};

		/**
		 * @return the layout (NULL if none)
		 */
		inline Layout *getLayout() const {
			return _layout;
		};
		/**
		 * Sets the used layout for this panel. This is only possible if no controls have been
		 * added yet.
		 *
		 * @param l the layout
		 */
		inline void setLayout(Layout *l) {
			if(_controls.size() > 0) {
				throw logic_error("This panel does already have controls;"
						" you can't change the layout afterwards");
			}
			_layout = l;
		};

		/**
		 * Performs a layout-calculation for this panel and all childs
		 */
		virtual void layout();

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
		
		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		/**
		 * Overwrite the paint-methods
		 */
		virtual void paint(Graphics &g);
		virtual void paintRect(Graphics &g,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		/**
		 * Adds the given control to this panel
		 *
		 * @param c the control
		 * @param pos the position-specification for the layout
		 */
		void add(Control &c,Layout::pos_type pos = 0);

	protected:
		virtual void resizeTo(gsize_t width,gsize_t height);
		virtual void moveTo(gpos_t x,gpos_t y);
		virtual void setRegion();

		/**
		 * @return the control that has the focus (not a panel!) or NULL if no one
		 */
		virtual const Control *getFocus() const {
			if(_focus)
				return _focus->getFocus();
			return NULL;
		};
		virtual Control *getFocus() {
			if(_focus)
				return _focus->getFocus();
			return NULL;
		};

	private:
		void passToCtrl(const MouseEvent &e,bool focus);
		virtual void setFocus(Control *c) {
			_focus = c;
			if(_parent)
				_parent->setFocus(this);
		};

	protected:
		Control *_focus;
		Color _bgColor;
		vector<Control*> _controls;
		Layout *_layout;
		sRectangle _updateRect;
	};

	ostream &operator<<(ostream &s,const Panel &p);
}

#endif /* PANEL_H_ */
