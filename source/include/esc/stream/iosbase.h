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

#include <sys/common.h>

namespace std {
class string;
}

namespace esc {

class OStream;
class IStream;

/**
 * The base-class of IStream and OStream that holds the state of the stream.
 */
class IOSBase {
protected:
	/**
	 * Collects all formatting parameters from a string
	 */
	class FormatParams {
	public:
		explicit FormatParams(const char *fmt);

		uint base() const {
			return _base;
		}
		uint flags() const {
			return _flags;
		}
		uint padding() const {
			return _pad;
		}
		void padding(uint pad) {
			_pad = pad;
		}
		uint precision() const {
			return _prec;
		}
		void precision(uint prec) {
			_prec = prec;
		}

	private:
		uint _base;
		uint _flags;
		uint _pad;
		uint _prec;
	};

	/**
	 * We use template specialization to do different formatting operations depending on the
	 * type given to fmt().
	 */
	template<typename T>
	class FormatImpl {
	};
	template<typename T>
	class FormatImplPtr {
	public:
		void read(IStream &os,const FormatParams &p,T &value);
		void write(OStream &os,const FormatParams &p,T value);
	};
	template<typename T>
	class FormatImplUint {
	public:
		void read(IStream &os,const FormatParams &p,T &value);
		void write(OStream &os,const FormatParams &p,const T &value);
	};
	template<typename T>
	class FormatImplInt {
	public:
		void read(IStream &os,const FormatParams &p,T &value);
		void write(OStream &os,const FormatParams &p,const T &value);
	};
	template<typename T>
	class FormatImplFloat {
	public:
		void read(IStream &os,const FormatParams &p,T &value);
		void write(OStream &os,const FormatParams &p,const T &value);
	};
	template<typename T>
	class FormatImplStr {
	public:
		void write(OStream &os,const FormatParams &p,const T &value);
	};
	template<typename T>
	class FormatImplStrObj {
	public:
		void read(IStream &os,const FormatParams &p,T &value);
		void write(OStream &os,const FormatParams &p,const T &value);
	};

public:
	enum Flags {
		PADRIGHT    = 1 << 0,
		FORCESIGN   = 1 << 1,
		SPACESIGN   = 1 << 2,
		PRINTBASE   = 1 << 3,
		PADZEROS    = 1 << 4,
		CAPHEX      = 1 << 5,
		POINTER     = 1 << 6,
		KEEPWS      = 1 << 7,
	};

	/**
	 * This class can be written into a stream to apply formatting while using the stream operators
	 * It will be used by the freestanding fmt() function, which makes it shorter because of template
	 * parameter type inference.
	 */
	template<typename T>
	class Format {
	public:
		explicit Format(const char *fmt,const T &value,uint pad = 0,uint prec = -1)
			: _fmt(fmt),_value(const_cast<T&>(value)),_pad(pad),_prec(prec) {
		}
		explicit Format(const char *fmt,T &value,uint pad = 0,uint prec = -1)
			: _fmt(fmt),_value(value),_pad(pad),_prec(prec) {
		}

		const char *fmt() const {
			return _fmt;
		}
		T &value() const {
			return _value;
		}
		uint padding() const {
			return _pad;
		}
		uint precision() const {
			return _prec;
		}

	private:
		const char *_fmt;
		T &_value;
		uint _pad;
		uint _prec;
	};

	enum {
		FL_EOF      = 1 << 0,
		FL_ERROR    = 1 << 1,
		FL_NOCLOSE  = 1 << 2,
	};

	explicit IOSBase() : _state() {
	}
	virtual ~IOSBase() {
	}

	/**
	 * Unsets all error-flags
	 */
	virtual void clear() {
		_state &= ~(FL_EOF | FL_ERROR);
	}

	/**
	 * @return true if everything is ok
	 */
	bool good() const {
		return (_state & (FL_EOF | FL_ERROR)) == 0;
	}
	/**
	 * @return true if EOF has been reached or an error occurred
	 */
	bool bad() const {
		return !good();
	}
	/**
	 * @return true if an error has occurred
	 */
	bool error() const {
		return _state & FL_ERROR;
	}
	/**
	 * @return true if EOF has been reached
	 */
	bool eof() const {
		return _state & FL_EOF;
	}

	bool operator!() const {
		return bad();
	}
	operator bool() const {
		return good();
	}

protected:
	uint _state;
};

template<>
class IOSBase::FormatImpl<void*> : public IOSBase::FormatImplPtr<const void*> {
};
template<>
class IOSBase::FormatImpl<const void*> : public IOSBase::FormatImplPtr<const void*> {
};
template<>
class IOSBase::FormatImpl<uchar> : public IOSBase::FormatImplUint<uchar> {
};
template<>
class IOSBase::FormatImpl<ushort> : public IOSBase::FormatImplUint<ushort> {
};
template<>
class IOSBase::FormatImpl<uint> : public IOSBase::FormatImplUint<uint> {
};
template<>
class IOSBase::FormatImpl<ulong> : public IOSBase::FormatImplUint<ulong> {
};
template<>
class IOSBase::FormatImpl<ullong> : public IOSBase::FormatImplUint<ullong> {
};
template<>
class IOSBase::FormatImpl<char> : public IOSBase::FormatImplInt<char> {
};
template<>
class IOSBase::FormatImpl<short> : public IOSBase::FormatImplInt<short> {
};
template<>
class IOSBase::FormatImpl<int> : public IOSBase::FormatImplInt<int> {
};
template<>
class IOSBase::FormatImpl<long> : public IOSBase::FormatImplInt<long> {
};
template<>
class IOSBase::FormatImpl<llong> : public IOSBase::FormatImplInt<llong> {
};
template<>
class IOSBase::FormatImpl<float> : public IOSBase::FormatImplFloat<float> {
};
template<>
class IOSBase::FormatImpl<double> : public IOSBase::FormatImplFloat<double> {
};
template<>
class IOSBase::FormatImpl<const char*> : public IOSBase::FormatImplStr<const char*> {
};
template<>
class IOSBase::FormatImpl<char*> : public IOSBase::FormatImplStr<char*> {
};
// this is necessary to be able to pass a string literal to fmt()
template<int X>
class IOSBase::FormatImpl<char [X]> : public IOSBase::FormatImplStr<char [X]> {
};
template<>
class IOSBase::FormatImpl<std::string> : public IOSBase::FormatImplStrObj<std::string> {
};
template<>
class IOSBase::FormatImpl<const std::string> : public IOSBase::FormatImplStrObj<const std::string> {
};

// unfortunatly, we have to add special templates for volatile :(
template<>
class IOSBase::FormatImpl<volatile void*> : public IOSBase::FormatImplPtr<volatile void*> {
};
template<>
class IOSBase::FormatImpl<volatile uchar> : public IOSBase::FormatImplUint<volatile uchar> {
};
template<>
class IOSBase::FormatImpl<volatile ushort> : public IOSBase::FormatImplUint<volatile ushort> {
};
template<>
class IOSBase::FormatImpl<volatile uint> : public IOSBase::FormatImplUint<volatile uint> {
};
template<>
class IOSBase::FormatImpl<volatile ulong> : public IOSBase::FormatImplUint<volatile ulong> {
};
template<>
class IOSBase::FormatImpl<volatile char> : public IOSBase::FormatImplInt<volatile char> {
};
template<>
class IOSBase::FormatImpl<volatile short> : public IOSBase::FormatImplInt<volatile short> {
};
template<>
class IOSBase::FormatImpl<volatile int> : public IOSBase::FormatImplInt<volatile int> {
};
template<>
class IOSBase::FormatImpl<volatile long> : public IOSBase::FormatImplInt<volatile long> {
};
template<>
class IOSBase::FormatImpl<volatile float> : public IOSBase::FormatImplFloat<volatile float> {
};
template<>
class IOSBase::FormatImpl<volatile const char*> : public IOSBase::FormatImplStr<volatile const char*> {
};
template<>
class IOSBase::FormatImpl<volatile char*> : public IOSBase::FormatImplStr<volatile char*> {
};
// this is necessary to be able to pass a string literal to fmt()
template<int X>
class IOSBase::FormatImpl<volatile char [X]> : public IOSBase::FormatImplStr<volatile char [X]> {
};

}
