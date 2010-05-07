/**
 * $Id: stream.h 647 2010-05-05 17:27:58Z nasmussen $
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

#ifndef STREAM_H_
#define STREAM_H_

#include <esc/common.h>
#include <esc/io.h>
#include <stdio.h>

#define IO_EOF		-1

#define OUTBUF_SIZE	512
#define INBUF_SIZE	64

namespace esc {
	/**
	 * The base-class of all streams
	 */
	class Stream {
	protected:
		/**
		 * None-public buffer-base-class. A buffer is simply the lowest instance for
		 * reading/writing one char from/to a source/destination which may provide
		 * buffering
		 */
		class Buffer {
		public:
			Buffer() {};
			virtual ~Buffer() {};

			/**
			 * @return the current position
			 */
			virtual u32 getPos() const = 0;

			/**
			 * @return whether we are at EOF
			 */
			virtual bool isEOF() const = 0;

			/**
			 * Flushes the buffer
			 *
			 * @return 0 on success
			 */
			virtual s32 flush() = 0;

			/**
			 * Reads one character from somewhere
			 *
			 * @return the character or EOF
			 */
			virtual char read() = 0;

			/**
			 * Reads <count> bytes into <buffer>
			 *
			 * @param buffer the buffer to fill
			 * @param count the number of bytes to read
			 * @return the number of read bytes or a negative error-code
			 */
			virtual s32 read(void *buffer,u32 count) = 0;

			/**
			 * Writes the character to somewhere
			 *
			 * @param c the character
			 * @return the character or EOF
			 */
			virtual char write(char c) = 0;

			/**
			 * Writes <count> bytes from <buffer>
			 *
			 * @param buffer the buffer
			 * @param count the number of bytes to write
			 * @return the number of written bytes or a negative error-code
			 */
			virtual s32 write(void *buffer,u32 count) = 0;

			/**
			 * Prints formated output to somewhere
			 *
			 * @param fmt the format
			 * @param ap the argument-list
			 * @return the number of written chars
			 */
			virtual s32 format(const char *fmt,va_list ap) = 0;
		};

		/**
		 * A concrete buffer that reads/writes from/to a string
		 */
		class StringBuffer : public Buffer {
		public:
			/**
			 * Constructs a new string-buffer with <buf> as source/destination and <max>
			 * as maximum length
			 *
			 * @param buf the string
			 * @param max the size of the string (0 = unlimited)
			 */
			StringBuffer(char *buf,s32 max = 0)
				// one additional slot for '\0'
				: _max(max - 1), _pos(0), _buffer(buf) {
			};
			/**
			 * Destroys the buffer
			 */
			virtual ~StringBuffer() {};

			virtual inline u32 getPos() const {
				return _pos;
			};
			virtual inline bool isEOF() const {
				return _max != -1 && _pos >= (u32)_max;
			};
			virtual s32 flush();
			virtual char read();
			virtual s32 read(void *buffer,u32 count);
			virtual char write(char c);
			virtual s32 write(void *buffer,u32 count);
			virtual s32 format(const char *fmt,va_list ap);

		/* no cloning */
		private:
			StringBuffer(const StringBuffer &b);
			StringBuffer &operator=(const StringBuffer &b);

		protected:
			s32 _max;
			u32 _pos;
			char *_buffer;
		};

		/**
		 * A concrete buffer that reads/writes from/to a file
		 */
		class FileBuffer : public StringBuffer {
		public:
			/**
			 * Creates a new file-buffer for the given file-descriptor and buffer-size
			 *
			 * @param fd the file-descriptor
			 * @param bufSize the size of the buffer
			 */
			FileBuffer(tFD fd,s32 bufSize) : StringBuffer(new char[bufSize],bufSize), _fd(fd) {
			};
			/**
			 * Destroys the buffer
			 */
			virtual ~FileBuffer() {
				delete _buffer;
			};

			/**
			 * @return the file-descriptor
			 */
			inline tFD getFileDesc() const {
				return _fd;
			};
			inline u32 getPos() const {
				u32 pos;
				if(tell(_fd,&pos) == 0)
					return pos;
				return IO_EOF;
			};
			inline bool isEOF() const {
				return eof(_fd);
			};
			s32 flush();
			char read();
			s32 read(void *buffer,u32 count);
			char write(char c);
			s32 write(void *buffer,u32 count);
			s32 format(const char *fmt,va_list ap);

		private:
			tFD _fd;
		};

		/**
		 * Protected constructor for sub-classes. Sets in and out to NULL so that the sub-class
		 * has to init them itself
		 */
		Stream() : _in(NULL), _out(NULL) {};

	public:
		/**
		 * Creates a new stream with the given in- and out-buffers
		 *
		 * @param in the in-buffer
		 * @param out the out-buffer
		 */
		Stream(Buffer *in,Buffer *out) : _in(in), _out(out) {};
		/**
		 * Destroys the stream
		 */
		virtual ~Stream() {};

		/**
		 * @return the current position
		 */
		u32 getPos() const;

		/**
		 * @return whether we are at EOF
		 */
		bool isEOF() const;

		/**
		 * Reads one character from the in-buffer
		 *
		 * @return the character or EOF
		 */
		char read();

		/**
		 * Reads <count> bytes into <buffer>
		 *
		 * @param buffer the buffer to fill
		 * @param count the number of bytes to read
		 * @return the number of read bytes or a negative error-code
		 */
		s32 read(void *buffer,u32 count);

		/**
		 * Writes the character to the out-stream
		 *
		 * @param c the character
		 * @return the character or EOF
		 */
		char write(char c);

		/**
		 * Writes the given string to the out-stream
		 *
		 * @param str the string
		 * @return the number of written chars
		 */
		s32 write(const char *str);

		/**
		 * Writes the given signed 32bit integer to the out-stream
		 *
		 * @param n the number
		 * @return the number of written chars
		 */
		s32 write(s32 n);

		/**
		 * Writes <count> bytes from <buffer>
		 *
		 * @param buffer the buffer
		 * @param count the number of bytes to write
		 * @return the number of written bytes or a negative error-code
		 */
		s32 write(void *buffer,u32 count);

		/**
		 * Prints formated output to the out-stream. The syntax is equal to the one of printf.
		 *
		 * @param fmt the format
		 * @return the number of written chars
		 */
		s32 format(const char *fmt,...);

	/* no cloning */
	private:
		Stream(const Stream &s);
		Stream &operator=(const Stream &s);

	protected:
		Buffer *_in;
		Buffer *_out;
	};

	/**
	 * A stream for reading and writing from/to a file
	 */
	class FileStream : public Stream {
	protected:
		FileStream() {};

	public:
		static const u8 READ	= IO_READ;
		static const u8 WRITE	= IO_WRITE;

		/**
		 * Opens the file with given mode and creates the corresponding buffers
		 *
		 * @param path the file-path
		 * @param mode the open-mode (READ|WRITE)
		 */
		FileStream(const char *path,u16 mode);

		/**
		 * Destroys the buffers and closes the file
		 */
		virtual ~FileStream();
	};

	/**
	 * A stream for input- and output with a vterm
	 */
	class IOStream : public FileStream {
	public:
		/**
		 * Creates a IO-stream for the given file-descriptor and mode
		 */
		IOStream(tFD fd,u16 mode);

		/**
		 * Destroys the stream
		 */
		~IOStream();
	};

	/**
	 * A stream for reading and writing from/to a string
	 */
	class StringStream : public Stream {
	public:
		/**
		 * Creates a new string-stream for given string and max-length
		 *
		 * @param buf the string
		 * @param max the length of the string
		 */
		StringStream(char *buf,s32 max);

		/**
		 * Destroys the stream
		 */
		~StringStream();
	};

	/**
	 * Writes the given char to the stream
	 */
	Stream &operator<<(Stream &s,char c);
	/**
	 * Writes the given string to the stream
	 */
	Stream &operator<<(Stream &s,const char *str);
	/**
	 * Writes the given number to the stream
	 */
	Stream &operator<<(Stream &s,u8 n);
	/**
	 * Writes the given number to the stream
	 */
	Stream &operator<<(Stream &s,s16 n);
	/**
	 * Writes the given number to the stream
	 */
	Stream &operator<<(Stream &s,u16 n);
	/**
	 * Writes the given number to the stream
	 */
	Stream &operator<<(Stream &s,s32 n);
	/**
	 * Writes the given number to the stream
	 */
	Stream &operator<<(Stream &s,u32 u);
	/**
	 * Reads one character into <c> from the stream
	 */
	Stream &operator>>(Stream &s,char &c);

	/**
	 * Stream for STDIN
	 */
	extern IOStream in;
	/**
	 * Stream for STDOUT
	 */
	extern IOStream out;
	/**
	 * Stream for STDERR
	 */
	extern IOStream err;

	/**
	 * A newline
	 */
	extern char endl;
}

#endif /* STREAM_H_ */
