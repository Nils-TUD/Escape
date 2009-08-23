/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/stream.h>
#include <string.h>

struct _sStreamBuf {
	tFD fd;
	u8 type;
	s32 pos;
	s32 max;
	char *str;
};

namespace esc {
	IOStream in(0,IOStream::READ);
	IOStream out(1,IOStream::WRITE);
	IOStream err(2,IOStream::WRITE);
	char endl = '\n';

	s32 Stream::StringBuffer::flush() {
		// nothing to do here
		return 0;
	}

	char Stream::StringBuffer::read() {
		if(_buffer[_pos] == '\0')
			return IO_EOF;
		return _buffer[_pos++];
	}

	s32 Stream::StringBuffer::read(void *buffer,u32 count) {
		if(_max != -1 && (_pos + count) > (u32)_max)
			count = _max - _pos;
		if(count > 0) {
			memcpy(buffer,_buffer + _pos,count);
			_pos += count;
		}
		return count;
	}

	char Stream::StringBuffer::write(char c) {
		if(_max != -1 && _pos >= (u32)_max)
			return IO_EOF;
		_buffer[_pos++] = c;
		// terminate string
		_buffer[_pos] = '\0';
		return c;
	}

	s32 Stream::StringBuffer::write(void *buffer,u32 count) {
		if(_max != -1 && (_pos + count) > (u32)_max)
			count = _max - _pos;
		if(count > 0) {
			memcpy(_buffer + _pos,buffer,count);
			_pos += count;
			_buffer[_pos] = '\0';
		}
		return count;
	}

	s32 Stream::StringBuffer::format(const char *fmt,va_list ap) {
		s32 res;
		// TODO this is an ugly hack! is there a better way?
		struct _sStreamBuf streamBuf;
		streamBuf.fd = -1;
		streamBuf.type = 0;	// 0 = string
		streamBuf.pos = _pos;
		streamBuf.max = _max;
		streamBuf.str = _buffer;
		res = doVfprintf(&streamBuf,fmt,ap);
		_pos = streamBuf.pos;
		return res;
	}

	s32 Stream::FileBuffer::flush() {
		s32 res = 0;
		if(_pos > 0) {
			if(::write(_fd,_buffer,_pos * sizeof(char)) < 0)
				res = IO_EOF;
			_pos = 0;
			// a process switch improves the performance by far :)
			if(res >= 0)
				yield();
		}
		return res;
	}

	char Stream::FileBuffer::read() {
		// TODO buffering
		char c;
		if(::read(_fd,&c,1) == 1)
			return c;
		return IO_EOF;
	}

	s32 Stream::FileBuffer::read(void *buffer,u32 count) {
		// TODO just temporary
		// TODO buffering
		char *cbuf = (char*)buffer;
		s32 c;
		u32 total = 0,amount;
		while(count > 0) {
			amount = MIN(1024,count);
			c = ::read(_fd,cbuf,amount);
			if(c < 0)
				return c;
			total += c;
			if(c < amount)
				break;
			cbuf += amount;
			count -= amount;
		}
		return total;
	}

	char Stream::FileBuffer::write(char c) {
		// TODO buffering
		if(::write(_fd,&c,1) == 1)
			return c;
		return IO_EOF;
	}

	s32 Stream::FileBuffer::write(void *buffer,u32 count) {
		// TODO just temporary
		// TODO buffering
		char *cbuf = (char*)buffer;
		s32 c;
		u32 total = 0,amount;
		while(count > 0) {
			amount = MIN(1024,count);
			c = ::write(_fd,cbuf,amount);
			if(c < 0)
				return c;
			total += c;
			if(c < amount)
				break;
			cbuf += amount;
			count -= amount;
		}
		return total;
	}

	s32 Stream::FileBuffer::format(const char *fmt,va_list ap) {
		s32 res;
		// TODO this is an ugly hack! is there a better way?
		struct _sStreamBuf streamBuf;
		streamBuf.fd = _fd;
		streamBuf.type = 1;	// 1 = file
		streamBuf.pos = _pos;
		streamBuf.max = _max;
		streamBuf.str = _buffer;
		res = doVfprintf(&streamBuf,fmt,ap);
		_pos = streamBuf.pos;
		return res;
	}

	u32 Stream::getPos() const {
		// it doesn't matter which buffer we use
		if(_out)
			return _out->getPos();
		return _in->getPos();
	}

	bool Stream::isEOF() const {
		// it doesn't matter which buffer we use
		if(_out)
			return _out->isEOF();
		return _in->isEOF();
	}

	char Stream::read() {
		if(!_in)
			return IO_EOF;
		return _in->read();
	}

	s32 Stream::read(void *buffer,u32 count) {
		return _in->read(buffer,count);
	}

	char Stream::write(char c) {
		if(!_out)
			return IO_EOF;
		return _out->write(c);
	}

	s32 Stream::write(const char *str) {
		char c;
		char *start = (char*)str;
		while((c = *str)) {
			// handle escape
			if(c == '\033') {
				if(_out->write(c) == IO_EOF)
					break;
				if(_out->write(*++str) == IO_EOF)
					break;
				if(_out->write(*++str) == IO_EOF)
					break;
			}
			else {
				if(_out->write(c) == IO_EOF)
					break;
			}
			str++;
		}
		return str - start;
	}

	s32 Stream::write(s32 n) {
		u32 c = 0;
		if(n < 0) {
			if(_out->write('-') == IO_EOF)
				return c;
			n = -n;
			c++;
		}

		if(n >= 10)
			c += write(n / 10);
		if(_out->write('0' + n % 10) == IO_EOF)
			return c;
		return c + 1;
	}

	s32 Stream::write(void *buffer,u32 count) {
		return _out->write(buffer,count);
	}

	s32 Stream::format(const char *fmt,...) {
		va_list ap;
		s32 res;
		va_start(ap,fmt);
		res = _out->format(fmt,ap);
		_out->flush();
		va_end(ap);
		return res;
	}

	FileStream::FileStream(const char *path,u16 mode) {
		tFD fd = open(path,mode);
		// TODO throw exception, if open failed
		if(fd < 0)
			err << "Open for '" << path << "' failed" << endl;
		if(mode & READ)
			_in = new FileBuffer(fd,INBUF_SIZE);
		if(mode & WRITE)
			_out = new FileBuffer(fd,OUTBUF_SIZE);
	}

	FileStream::~FileStream() {
		// close file
		if(_in)
			close(((FileBuffer*)_in)->getFileDesc());
		else if(_out)
			close(((FileBuffer*)_out)->getFileDesc());

		// delete buffers
		delete _in;
		delete _out;
	}

	IOStream::IOStream(tFD fd,u16 mode) {
		if(mode & READ)
			_in = new FileBuffer(fd,INBUF_SIZE);
		if(mode & WRITE)
			_out = new FileBuffer(fd,OUTBUF_SIZE);
	}

	IOStream::~IOStream() {
		delete _in;
		delete _out;
	}

	StringStream::StringStream(char *buf,s32 max) {
		StringBuffer *buffer = new StringBuffer(buf,max);
		_in = buffer;
		_out = buffer;
	}

	StringStream::~StringStream() {
		delete _in;
	}

	Stream &operator<<(Stream &s,char c) {
		s.write(c);
		return s;
	}

	Stream &operator<<(Stream &s,const char *str) {
		s.write(str);
		return s;
	}

	Stream &operator<<(Stream &s,u8 n) {
		s.write((s32)n);
		return s;
	}

	Stream &operator<<(Stream &s,s16 n) {
		s.write((s32)n);
		return s;
	}

	Stream &operator<<(Stream &s,u16 n) {
		s.write((s32)n);
		return s;
	}

	Stream &operator<<(Stream &s,s32 n) {
		s.write(n);
		return s;
	}

	Stream &operator<<(Stream &s,u32 u) {
		s.write((s32)u);
		return s;
	}

	Stream &operator>>(Stream &s,char &c) {
		c = s.read();
		return s;
	}
}
