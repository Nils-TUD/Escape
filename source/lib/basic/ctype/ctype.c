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

#include <ctype.h>
#include <stddef.h>

uchar ctypetbl[] = {
	/* 00 .. 03: NUL, SOH, STX, ETX */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 04 .. 07: EOT, ENQ, ACK, \a */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 08 .. 0B: \b, \t, \n, \v */
	CT_CTRL, CT_CTRL | CT_BLANK, CT_CTRL | CT_SPACE, CT_CTRL | CT_SPACE,
	/* 0C .. 0F: \f, \r, SO, SI */
	CT_CTRL | CT_SPACE, CT_CTRL | CT_SPACE, CT_CTRL, CT_CTRL,
	/* 10 .. 13: DLE, DC1, DC2, DC3 */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 14 .. 17: DC4, NAK, SYN, ETB */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 18 .. 1B: CAN, EM, SUB, ESC */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 1C .. 1F: FS, GS, RS, US */
	CT_CTRL, CT_CTRL, CT_CTRL, CT_CTRL,
	/* 20 .. 23: SP, !, ", # */
	CT_BLANK, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 24 .. 27: $, %, &, ' */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 28 .. 2B: (, ), *, + */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 2C .. 2F: ,, -, ., / */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 30 .. 33: 0, 1, 2, 3 */
	CT_NUMERIC, CT_NUMERIC, CT_NUMERIC, CT_NUMERIC,
	/* 34 .. 37: 4, 5, 6, 7 */
	CT_NUMERIC, CT_NUMERIC, CT_NUMERIC, CT_NUMERIC,
	/* 38 .. 3B: 8, 9, :, ; */
	CT_NUMERIC, CT_NUMERIC, CT_PUNCT, CT_PUNCT,
	/* 3C .. 3F: <, =, >, ? */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 40 .. 43: @, A, B, C */
	CT_PUNCT, CT_UPPER | CT_HEX, CT_UPPER | CT_HEX, CT_UPPER | CT_HEX,
	/* 44 .. 47: D, E, F, G */
	CT_UPPER | CT_HEX, CT_UPPER | CT_HEX, CT_UPPER | CT_HEX, CT_UPPER,
	/* 48 .. 4B: H, I, J, K */
	CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
	/* 4C .. 4F: L, M, N, O */
	CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
	/* 50 .. 53: P, Q, R, S */
	CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
	/* 54 .. 57: T, U, V, W */
	CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
	/* 58 .. 5B: X, Y, Z, [ */
	CT_UPPER, CT_UPPER, CT_UPPER, CT_PUNCT,
	/* 5C .. 5F: \, ], ^, _ */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_PUNCT,
	/* 60 .. 63: `, a, b, c */
	CT_PUNCT, CT_LOWER | CT_HEX, CT_LOWER | CT_HEX, CT_LOWER | CT_HEX,
	/* 64 .. 67: d, e, f, g */
	CT_LOWER | CT_HEX, CT_LOWER | CT_HEX, CT_LOWER | CT_HEX, CT_LOWER,
	/* 68 .. 6B: h, i, j, k */
	CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
	/* 6C .. 6F: l, m, n, o */
	CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
	/* 70 .. 73: p, q, r, s */
	CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
	/* 74 .. 77: t, u, v, w */
	CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
	/* 78 .. 7B: x, y, z, { */
	CT_LOWER, CT_LOWER, CT_LOWER, CT_PUNCT,
	/* 7C .. 7F: |, }, ~, ?? */
	CT_PUNCT, CT_PUNCT, CT_PUNCT, CT_CTRL,
	/* 80 .. 83: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* 84 .. 87: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* 88 .. 8B: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* 8C .. 8F: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* 90 .. 93: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* 94 .. 97: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* 98 .. 9B: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* 9C .. 9F: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* A0 .. A3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* A4 .. A7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* A8 .. AB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* AC .. AF: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* B0 .. B3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* B4 .. B7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* B8 .. BB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* BC .. BF: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* C0 .. C3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* C4 .. C7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* C8 .. CB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* CC .. CF: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* D0 .. D3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* D4 .. D7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* D8 .. DB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* DC .. DF: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* E0 .. E3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* E4 .. E7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* E8 .. EB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* EC .. EF: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* F0 .. F3: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* F4 .. F7: ??, ??, ??, ??*/
	0, 0, 0, 0,
	/* F8 .. FB: ??, ??, ??, ?? */
	0, 0, 0, 0,
	/* FC .. FF: ??, ??, ??, EOF */
	0, 0, 0, CT_CTRL,
};
