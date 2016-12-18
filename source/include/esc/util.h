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
#include <sys/arch.h>

namespace esc {

class Util {
	Util() = delete;

public:
    template<typename T>
    static constexpr T abs(T x) {
        return x < 0 ? -x : x;
    }

    template<typename T>
    static constexpr T min(T a, T b) {
        return a < b ? a : b;
    }
    template<typename T>
    static constexpr T max(T a, T b) {
        return a < b ? b : a;
    }

    /**
     * Assuming that <startx> < <endx> and <endx> is not included (that means with start=0 and end=10
     * 0 .. 9 is used), the macro determines whether the two ranges overlap anywhere.
     */
    template<typename T>
    static bool overlap(T start1, T end1, T start2, T end2) {
        return (start1 >= start2 && start1 < end2) ||   // start in range
            (end1 > start2 && end1 <= end2) ||          // end in range
            (start1 < start2 && end1 > end2);           // complete overlapped
    }

    template<typename T>
    static constexpr T round_up(T value, size_t align) {
        return (value + align - 1) & ~static_cast<T>(align - 1);
    }
    template<typename T>
    static constexpr T round_dn(T value, size_t align) {
        return value & ~static_cast<T>(align - 1);
    }

    template<typename T>
    static constexpr T round_page_up(T value) {
    	return round_up(value,PAGE_SIZE);
    }
    template<typename T>
    static constexpr T round_page_dn(T value) {
    	return round_dn(value,PAGE_SIZE);
    }
};

}
