/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <esc/common.h>

#include "blockfile.h"
#include "datacon.h"
#include "ctrlcon.h"

class File : public BlockFile {
public:
	explicit File(const std::string &path,CtrlCon *ctrl)
			: _offset(), _path(path), _ctrl(ctrl), _data(new DataCon(ctrl)) {
		startTransfer(_offset);
	}
	virtual ~File() {
		delete _data;
		_ctrl->readReply();
	}

	virtual size_t read(void *buf,size_t offset,size_t count) {
		if(offset != _offset) {
			delete _data;
			_ctrl->readReply();
			_data = new DataCon(_ctrl);
			_offset = offset;
			startTransfer(_offset);
		}
		size_t res = _data->read(buf,count);
		_offset += res;
		return res;
	}

private:
	void startTransfer(size_t offset) {
		if(offset != 0) {
			char buf[32];
			snprintf(buf,sizeof(buf),"%zu",offset);
			_ctrl->execute(CtrlCon::CMD_REST,buf);
		}
		_ctrl->execute(CtrlCon::CMD_RETR,_path.c_str());
	}

	size_t _offset;
	const std::string &_path;
	CtrlCon *_ctrl;
	DataCon *_data;
};
