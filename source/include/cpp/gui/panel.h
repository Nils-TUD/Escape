/**
 * $Id$
 */

#pragma once

#include <esc/common.h>
#include <gui/event/event.h>
#include <gui/layout/layout.h>
#include <gui/control.h>
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

	public:
		/**
		 * Creates an empty panel with given layout. Note that the layout will be owned by the
		 * panel afterwards. So, it will delete it when the panel is deleted.
		 *
		 * @param l the layout (may be NULL)
		 */
		Panel(Layout *l = NULL)
			: Control(), _focus(NULL), _controls(), _layout(l), _updateRect() {
		}
		/**
		 * Creates an empty panel at given position, with given size and with given layout. Note
		 * that the layout will be owned by the panel afterwards. So, it will delete it when the
		 * panel is deleted.
		 *
		 * @param pos the position
		 * @param size the size
		 * @param l the layout (may be NULL)
		 */
		Panel(const Pos &pos,const Size &size,Layout *l = NULL)
			: Control(pos,size), _focus(NULL), _controls(), _layout(l), _updateRect() {
		}
		/**
		 * Destructor
		 */
		virtual ~Panel() {
			for(std::vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it)
				delete (*it);
			delete _layout;
		}

		virtual Size getPrefSize() const {
			if(_layout) {
				gsize_t pad = getTheme().getPadding();
				return _layout->getPreferredSize() + Size(pad * 2,pad * 2);
			}
			return Size();
		}
		virtual Size getUsedSize(const Size &avail) const {
			if(_layout) {
				gsize_t pad = getTheme().getPadding();
				Size padsize = Size(pad * 2,pad * 2);
				return _layout->getUsedSize(subsize(avail,padsize)) + padsize;
			}
			return UIElement::getUsedSize(avail);
		}

		/**
		 * @return the layout (NULL if none)
		 */
		Layout *getLayout() const {
			return _layout;
		}
		/**
		 * Sets the used layout for this panel. This is only possible if no controls have been
		 * added yet.
		 *
		 * @param l the layout
		 */
		void setLayout(Layout *l) {
			if(_controls.size() > 0) {
				throw std::logic_error("This panel does already have controls;"
						" you can't change the layout afterwards");
			}
			_layout = l;
		}

		/**
		 * Performs a layout-calculation for this panel and all childs
		 */
		virtual void layout();
		
		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		/**
		 * Adds the given control to this panel. That means, the panel will own the control
		 * afterwards. So, it will delete it when the panel is deleted.
		 *
		 * @param c the control
		 * @param pos the position-specification for the layout
		 */
		void add(Control *c,Layout::pos_type pos = 0);

		/**
		 * Removes the given control from this panel. That means, the panel will no longer own
		 * the control afterwards. So, you can add it to a different panel afterwards, for example.
		 *
		 * @param c the control
		 * @param pos the position-specification for the layout
		 */
		void remove(Control *c,Layout::pos_type pos = 0);

	protected:
		virtual void paint(Graphics &g);
		virtual void paintRect(Graphics &g,const Pos &pos,const Size &size);

		virtual void resizeTo(const Size &size);
		virtual void moveTo(const Pos &pos);
		virtual void setRegion();

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
		}

	private:
		void passToCtrl(const MouseEvent &e,bool focus);
		virtual void setFocus(Control *c) {
			_focus = c;
			if(_parent)
				_parent->setFocus(this);
		}

	protected:
		Control *_focus;
		std::vector<Control*> _controls;
		Layout *_layout;
		sRectangle _updateRect;
	};

	std::ostream &operator<<(std::ostream &s,const Panel &p);
}
