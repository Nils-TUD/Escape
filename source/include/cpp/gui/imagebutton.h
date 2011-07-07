/**
 * $Id$
 */

#ifndef IMAGEBUTTON_H_
#define IMAGEBUTTON_H_

#include <esc/common.h>
#include <gui/button.h>
#include <gui/image.h>

namespace gui {
	class ImageButton : public Button {
	public:
		ImageButton(Image* img,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Button(x,y,width,height), _img(img) {
		};
		ImageButton(const ImageButton &b)
			: Button(b), _img(b._img) {
		};
		virtual ~ImageButton() {
		};
		ImageButton &operator=(const ImageButton &b) {
			if(this == &b)
				return *this;
			Button::operator =(b);
			this->_img = b._img;
			return *this;
		}

		inline Image *getImage() const {
			return _img;
		};

		virtual void paintBackground(Graphics &g);

	private:
		Image *_img;
	};
}

#endif /* IMAGEBUTTON_H_ */
