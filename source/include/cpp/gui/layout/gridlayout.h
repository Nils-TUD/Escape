/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#ifndef GRIDLAYOUT_H_
#define GRIDLAYOUT_H_

#include <esc/common.h>
#include <gui/layout/layout.h>
#include <gui/control.h>
#include <map>

namespace gui {
	/**
	 * Should be used to specify positions in a gridlayout.
	 */
	class GridPos {
		static const unsigned MAX_COLS	= 1024;

	public:
		explicit GridPos(unsigned pos) : _col(pos % MAX_COLS), _row(pos / MAX_COLS) {
		}
		explicit GridPos(unsigned c,unsigned r) : _col(c), _row(r) {
		}

		operator int() const {
			return _row * MAX_COLS + _col;
		}
		unsigned col() const {
			return _col;
		}
		unsigned row() const {
			return _row;
		}

	private:
		unsigned _col;
		unsigned _row;
	};

	/**
	 * The gridlayout builds a table with specified number of columns and rows.
	 */
	class GridLayout : public Layout {
	public:
		/**
		 * Constructor
		 *
		 * @param cols the number of columns
		 * @param rows the number of rows
		 */
		GridLayout(unsigned cols,unsigned rows)
			: Layout(), _cols(cols), _rows(rows), _p(), _ctrls() {
		};
		/**
		 * Destructor
		 */
		virtual ~GridLayout() {
		};

		virtual void add(Panel *p,Control *c,pos_type pos);
		virtual void remove(Panel *p,Control *c,pos_type pos);

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;

		virtual gsize_t getPreferredWidth() const;
		virtual gsize_t getPreferredHeight() const;

		virtual void rearrange();

	private:
		gsize_t getMaxWidth() const;
		gsize_t getMaxHeight() const;

	private:
		unsigned _cols;
		unsigned _rows;
		Control *_p;
		std::map<int,Control*> _ctrls;
	};
}

#endif /* GRIDLAYOUT_H_ */
