/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of M3 (Microkernel for Minimalist Manycores).
 *
 * M3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * M3 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <sys/common.h>
#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <stdio.h>

namespace esc {

/**
 * FStream is an input- and output-stream for files. It uses FILE as a backend.
 */
class FStream : public IStream, public OStream {
public:
	/**
	 * Opens <filename> with given mode
	 *
	 * @param filename the file to open
	 * @param mode the open-mode (see fopen)
	 */
	explicit FStream(const char *filename,const char *mode = "r+")
		: IStream(), OStream(), _file(fopen(filename,mode)) {
		if(_file == NULL)
			_state |= FL_NOCLOSE | FL_ERROR;
	}
	/**
	 * Uses the given file as the backend. The file is not closed if this object is destructed.
	 *
	 * @param file the file to use
	 */
	explicit FStream(FILE *file) : IStream(), OStream(), _file(file) {
		_state |= FL_NOCLOSE;
	}
	/**
	 * Uses the given file-descriptor as the backend. The file is not closed if this object is
	 * destructed.
	 *
	 * @param fd the file-descriptor to use
	 */
	explicit FStream(int fd,const char *mode) : IStream(), OStream(), _file(fattach(fd,mode)) {
		if(_file == NULL)
			_state |= FL_NOCLOSE | FL_ERROR;
	}
	virtual ~FStream() {
		if(~_state & FL_NOCLOSE)
			fclose(_file);
	}

	FStream(FStream &&f) : IStream(std::move(f)), OStream(std::move(f)), _file(f._file) {
		f._state |= FL_NOCLOSE;
	}
	FStream &operator=(FStream &&f) {
		if(&f != this) {
			IStream::operator=(std::move(f));
			OStream::operator=(std::move(f));
			_file = f._file;
			f._state |= FL_NOCLOSE;
		}
		return *this;
	}

	/**
	 * @return the file-descriptor
	 */
	int fd() const {
		return fileno(_file);
	}

	/**
	 * @return the number of bytes that are readable from the internal buffer
	 */
	size_t inbuflen() const {
		return favail(_file);
	}

	/**
	 * Seeks to the given position.
	 *
	 * @param offset the offset to seek to (meaning depends on <whence>)
	 * @param whence the seek type (SEEK_*)
	 */
	void seek(off_t offset,int whence) {
		fseek(_file,offset,whence);
		setflags();
	}

	virtual char read() override {
		char c = fgetc(_file);
		setflags();
		return c;
	}
	virtual size_t read(void *dst,size_t count) override {
		ssize_t res = fread(dst,1,count,_file);
		setflags();
		return res < 0 ? 0 : res;
	}
	virtual bool putback(char c) override {
		return ungetc(c,_file) == 0;
	}

	virtual void write(char c) override {
		fputc(c,_file);
		setflags();
	}
	virtual size_t write(const void *src,size_t count) override {
		ssize_t res = fwrite(src,1,count,_file);
		setflags();
		return res < 0 ? 0 : res;
	}
	virtual void flush() override {
		fflush(_file);
	}
	virtual void clear() override {
		IOSBase::clear();
		clearerr(_file);
	}

private:
	void setflags() {
		if(feof(_file))
			_state |= FL_EOF;
		if(ferror(_file))
			_state |= FL_ERROR;
	}

	FILE *_file;
};

}
