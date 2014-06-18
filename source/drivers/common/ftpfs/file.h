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
	explicit File(const std::string &path,size_t size,const CtrlConRef &ctrl)
			: _reading(false), _offset(-1), _shm(), _shmsize(), _total(size), _path(path),
			  _ctrlRef(ctrl), _ctrl(), _data() {
	}
	virtual ~File() {
		if(_data) {
			try {
				stop();
			}
			catch(...) {
			}
		}
	}

	virtual size_t read(void *buf,size_t offset,size_t count) {
		if(!_reading || offset != _offset)
			startTransfer(offset,true);
		size_t res = _data->read(buf,count);
		_offset += res;
		return res;
	}

	virtual void write(const void *buf,size_t offset,size_t count) {
		if(_reading || offset != _offset)
			startTransfer(offset,false);
		_data->write(buf,count);
		_offset += count;
	}

	virtual int sharemem(void *mem,size_t size) {
		if(_shm)
			return -EEXIST;
		_shm = mem;
		_shmsize = size;
		return 0;
	}

private:
	void startTransfer(size_t offset,bool reading) {
		// if not done yet, request the control-connection for ourself. we'll release it in our
		// destructor, i.e. when the transfer is finished
		if(!_ctrl)
			_ctrl = _ctrlRef.request();
		else if(_data) {
			stop();
			_data = NULL;
		}
		_data = new DataCon(_ctrlRef);
		if(_shm)
			_data->sharemem(_shm,_shmsize);
		_offset = offset;
		_reading = reading;

		// if RETR or STOR fail for example, we won't get a reply on the control-channel
		// so, better destroy the data-channel. we can't use it anyway.
		try {
			if(offset != 0) {
				char buf[32];
				snprintf(buf,sizeof(buf),"%zu",offset);
				_ctrl->execute(CtrlCon::CMD_REST,buf);
			}
			_ctrl->execute(_reading ? CtrlCon::CMD_RETR : CtrlCon::CMD_STOR,_path.c_str());
		}
		catch(...) {
			delete _data;
			_data = NULL;
			throw;
		}
	}

	void stop() {
		bool aborting = _offset == (size_t)-1 || _offset < _total;
		if(aborting)
			_data->abort();
		delete _data;
		if(_reading && aborting)
			_ctrl->abort();
		else
			_ctrl->readReply();
	}

	bool _reading;
	size_t _offset;
	void *_shm;
	size_t _shmsize;
	size_t _total;
	const std::string &_path;
	CtrlConRef _ctrlRef;
	CtrlCon *_ctrl;
	DataCon *_data;
};
