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

#include <esc/ipc/device.h>
#include <esc/ipc/vtermdevice.h>
#include <esc/ringbuffer.h>
#include <sys/proc.h>
#include <keymap/keymap.h>
#include <mutex>
#include <signal.h>

namespace esc {

class SerialTermDevice : public Device {
public:
	static const size_t BUF_SIZE	= 4096;
	static const size_t INBUF_SIZE	= 1024;

	explicit SerialTermDevice(const char *name,mode_t perm)
		: Device(name,perm,DEV_TYPE_CHAR,DEV_CANCEL | DEV_READ | DEV_WRITE | DEV_CLOSE),
		  _shpid(), _flags((1 << VTerm::FL_ECHO) | (1 << VTerm::FL_READLINE)), _eof(),
		  _keymap(Keymap::getDefault()), _mutex(),
		  _requests(std::make_memfun(this,&SerialTermDevice::handleRead)),
		  _inbuf(INBUF_SIZE,RB_OVERWRITE) {
		/* init screen mode */
		_mode.id = 1;
		_mode.cols = 80;
		_mode.rows = 25;
		_mode.width = 0;
		_mode.height = 0;
		_mode.type = Screen::MODE_TYPE_TUI;
		_mode.mode = Screen::MODE_TEXT;

		set(MSG_DEV_CANCEL,std::make_memfun(this,&SerialTermDevice::cancel));

		set(MSG_FILE_READ,std::make_memfun(this,&SerialTermDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&SerialTermDevice::write));

		set(MSG_VT_GETFLAG,std::make_memfun(this,&SerialTermDevice::getFlag));
		set(MSG_VT_SETFLAG,std::make_memfun(this,&SerialTermDevice::setFlag));
		set(MSG_VT_SHELLPID,std::make_memfun(this,&SerialTermDevice::setShellPid));
		set(MSG_VT_ISVTERM,std::make_memfun(this,&SerialTermDevice::isVTerm));
		set(MSG_VT_BACKUP,std::make_memfun(this,&SerialTermDevice::backup));
		set(MSG_VT_RESTORE,std::make_memfun(this,&SerialTermDevice::restore));

		set(MSG_SCR_GETMODE,std::make_memfun(this,&SerialTermDevice::getMode));
	}

	void cancel(IPCStream &is) {
		msgid_t mid;
		is >> mid;

		int res;
		// we answer write-requests always right away, so let the kernel just wait for the response
		if((mid & 0xFFFF) == MSG_FILE_WRITE)
			res = 1;
		else if((mid & 0xFFFF) != MSG_FILE_READ)
			res = -EINVAL;
		else {
			std::lock_guard<std::mutex> guard(_mutex);
			if(_inbuf.length() > 0 || _eof)
				res = 1;
			else
				res = _requests.cancel(mid);
		}
		is << res << Reply();
	}

	void read(IPCStream &is) {
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		char *data = r.count <= BUF_SIZE ? _buffer : new char[r.count];

		std::lock_guard<std::mutex> guard(_mutex);
		if(_requests.size() == 0 && (_inbuf.length() > 0 || _eof))
			handleRead(is.fd(),is.msgid(),data,r.count);
		else
			_requests.enqueue(Request(is.fd(),is.msgid(),data,r.count));
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;
		assert(!is.error());

		DataBuf buf(r.count < BUF_SIZE ? _buffer : new char[r.count + 1],r.count >= BUF_SIZE);

		is >> ReceiveData(buf.data(),r.count);
		buf.data()[r.count] = '\0';

		{
			std::lock_guard<std::mutex> guard(_mutex);
			writeStr(buf.data());
		}

		is << FileWrite::Response::success(r.count) << Reply();

		std::lock_guard<std::mutex> guard(_mutex);
		checkPending();
	}

	void backup(IPCStream &is) {
		is << errcode_t(0) << Reply();
	}
	void restore(IPCStream &is) {
		is << errcode_t(0) << Reply();
	}
	void getMode(IPCStream &is) {
		is << ValueResponse<Screen::Mode>::success(_mode) << Reply();
	}
	void setShellPid(IPCStream &is) {
		pid_t pid;
		is >> pid;
		_shpid = pid;
		is << errcode_t(0) << Reply();
	}
	void isVTerm(IPCStream &is) {
		is << errcode_t(1) << Reply();
	}
	void getFlag(IPCStream &is) {
		VTerm::Flag flag;
		is >> flag;
		is << ValueResponse<bool>::success((_flags & (1 << flag)) ? 1 : 0) << Reply();
	}
	void setFlag(IPCStream &is) {
		bool val;
		VTerm::Flag flag;
		is >> flag >> val;
		_flags &= ~(1 << flag);
		if(val)
			_flags |= 1 << flag;
		is << errcode_t(0) << Reply();
	}

	void handleChar(char c) {
		if(c == '\r')
		 	c = '\n';
		else if(c == '\n')
			return;

		/* ^C */
		if(c == 0x3) {
			/* send interrupt to shell */
			if(_shpid) {
				if(kill(_shpid,SIGINT) < 0)
					printe("Unable to send SIGINT to %d",_shpid);
			}
			return;
		}
		/* ^D */
		else if(c == 0x4) {
			std::lock_guard<std::mutex> guard(_mutex);
			_eof = true;
			checkPending();
			return;
		}

		uchar modifier,keycode;
		if(c == '\e') {
			c = '^';
			keycode = VK_ACCENT;
			modifier = 0;
		}
		else
			keycode = _keymap->translateChar(c,&modifier);

		if(keycode != VK_NOKEY) {
			if(~_flags & (1 << VTerm::FL_READLINE))
				writeRL(c,keycode,modifier);
			else
				writeDef(c);
		}
	}

	virtual void writeChar(char c) = 0;

private:
	void checkPending() {
		_requests.handle();
	}

	void writeStr(const char *str) {
		static char escbuf[32];
		static size_t esclen = 0;
		int n1,n2,n3;

		// are we in an escape code?
		if(esclen > 0) {
			while(*str && esclen < ARRAY_SIZE(escbuf)) {
				escbuf[esclen++] = *str;
				escbuf[esclen] = '\0';
				const char *tmp = escbuf + 1;
				str++;
				if(escc_get(&tmp,&n1,&n2,&n3) != ESCC_INCOMPLETE) {
					esclen = 0;
					break;
				}
			}
			// if it's too long, stop here
			if(esclen == ARRAY_SIZE(escbuf))
				esclen = 0;
		}

		while(*str) {
			// ignore escape codes
			if(*str == '\033') {
				str++;
				if(escc_get(&str,&n1,&n2,&n3) == ESCC_INCOMPLETE) {
					strnzcpy(escbuf,str - 1,sizeof(escbuf));
					esclen = strlen(escbuf);
					return;
				}
				continue;
			}
			writeChar(*str++);
		}
	}

	void writeRL(char c,uchar keycode,uchar modifier) {
		char escape[SSTRLEN("\033[kc;123;123;15]") + 1];
		/* we want to treat the character as unsigned here and extend it to 32bit */
		uint code = *(uchar*)&c;
		snprintf(escape,sizeof(escape),"\033[kc;%u;%u;%u]",code,keycode,modifier);

		std::lock_guard<std::mutex> guard(_mutex);
		_inbuf.writen(escape,strlen(escape));
		checkPending();
	}

	void writeDef(char c) {
		if(c == '\a' || c == '\t')
			return;

		{
			std::lock_guard<std::mutex> guard(_mutex);
			_inbuf.write(c);
			checkPending();
		}

		if(_flags & (1 << VTerm::FL_ECHO))
			writeChar(c);
	}

	bool handleRead(int fd,msgid_t mid,char *data,size_t count) {
		if(!_eof && _inbuf.length() == 0)
			return false;

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		IPCStream is(fd,buffer,sizeof(buffer),mid);

		ssize_t res = std::min(count,_inbuf.length());
		if(res > 0)
			_inbuf.readn(data,res);
		is << FileRead::Response::result(res) << Reply();
		if(res > 0)
			is << ReplyData(data,res);
		else
			_eof = false;
		if(count > BUF_SIZE)
			delete[] data;
		return true;
	}

	int _shpid;
	uint _flags;
	bool _eof;
	Keymap *_keymap;
	std::mutex _mutex;
	RequestQueue _requests;
	RingBuffer<char> _inbuf;
	Screen::Mode _mode;
	char _buffer[BUF_SIZE];
};

}
