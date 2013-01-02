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
		ImageButton(Image *img,bool border = true)
			: Button(""), _img(img), _border(border) {
		};
		ImageButton(Image *img,const Pos &pos,const Size &size,bool border = true)
			: Button("",pos,size), _img(img), _border(border) {
		};

		Image *getImage() const {
			return _img;
		};

		virtual Size getPrefSize() const;
		virtual void paintBorder(Graphics &g);
		virtual void paintBackground(Graphics &g);

	private:
		Image *_img;
		bool _border;
	};
}
