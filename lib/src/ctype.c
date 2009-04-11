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

#include <types.h>
#include <ctype.h>

bool isalnum(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

bool isalpha(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool iscntrl(s32 c) {
	return c < ' ';
}

bool isdigit(s32 c) {
	return (c >= '0' && c <= '9');
}

bool islower(s32 c) {
	return (c >= 'a' && c <= 'z');
}

bool isprint(s32 c) {
	return c >= ' ' && c <= '~';
}

bool ispunct(s32 c) {
	return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') ||
		(c >= '{' && c <= '~');
}

bool isspace(s32 c) {
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}

bool isupper(s32 c) {
	return (c >= 'A' && c <= 'Z');
}

bool isxdigit(s32 c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

s32 tolower(s32 ch) {
	if(ch >= 'A' && ch <= 'Z')
		return ch - ('A' - 'a');
	return ch;
}

s32 toupper(s32 ch) {
	if(ch >= 'a' && ch <= 'z')
		return ch + ('A' - 'a');
	return ch;
}
