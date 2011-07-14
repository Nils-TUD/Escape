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
		ImageButton(Image *img,bool border = true)
			: Button(""), _img(img), _border(border) {
		};
		ImageButton(Image *img,gpos_t x,gpos_t y,gsize_t width,gsize_t height,bool border = true)
			: Button("",x,y,width,height), _img(img), _border(border) {
		};
		ImageButton(const ImageButton &b)
			: Button(b), _img(b._img), _border(b._border) {
		};
		virtual ~ImageButton() {
		};
		ImageButton &operator=(const ImageButton &b) {
			if(this == &b)
				return *this;
			Button::operator =(b);
			_img = b._img;
			_border = b._border;
			return *this;
		}

		inline Image *getImage() const {
			return _img;
		};

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;
		virtual void paintBorder(Graphics &g);
		virtual void paintBackground(Graphics &g);

	private:
		Image *_img;
		bool _border;
	};
}

#endif /* IMAGEBUTTON_H_ */
