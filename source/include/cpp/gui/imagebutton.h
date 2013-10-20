/**
 * $Id$
 */

#pragma once

#include <esc/common.h>
#include <gui/image/image.h>
#include <gui/button.h>

namespace gui {
	class ImageButton : public Button {
	public:
		ImageButton(std::shared_ptr<Image> img,bool border = true)
			: Button(""), _img(img), _border(border) {
		}
		ImageButton(std::shared_ptr<Image> img,const Pos &pos,const Size &size,bool border = true)
			: Button("",pos,size), _img(img), _border(border) {
		}

		std::shared_ptr<Image> getImage() const {
			return _img;
		}

		virtual void paintBorder(Graphics &g);
		virtual void paintBackground(Graphics &g);

	private:
		virtual Size getPrefSize() const;

		std::shared_ptr<Image> _img;
		bool _border;
	};
}
