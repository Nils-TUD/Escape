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

#include <esc/stream/iosbase.h>

namespace esc {

IOSBase::FormatParams::FormatParams(const char *fmt)
		: _base(10),_flags(0),_pad(0),_prec(-1) {
	// read flags
	bool readFlags = true;
	while(readFlags) {
		switch(*fmt) {
			case '-':
				_flags |= PADRIGHT;
				fmt++;
				break;
			case '+':
				_flags |= FORCESIGN;
				fmt++;
				break;
			case ' ':
				_flags |= SPACESIGN;
				fmt++;
				break;
			case '#':
				_flags |= PRINTBASE;
				fmt++;
				break;
			case '0':
				_flags |= PADZEROS;
				fmt++;
				break;
			case 'w':
				_flags |= KEEPWS;
				fmt++;
				break;
			default:
				readFlags = false;
				break;
		}
	}

	// read base
	switch(*fmt) {
		case 'X':
		case 'x':
			if(*fmt == 'X')
				_flags |= CAPHEX;
			_base = 16;
			break;
		case 'o':
			_base = 8;
			break;
		case 'b':
			_base = 2;
			break;
		case 'p':
			_flags |= POINTER;
			break;
	}
}

}
