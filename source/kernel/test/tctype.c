/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <sys/common.h>
#include <sys/video.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "tctype.h"
#include <esc/test.h>

static void test_ctype(void);
static int test_isalnum(int c);
static int test_isalpha(int c);
static int test_iscntrl(int c);
static int test_isdigit(int c);
static int test_islower(int c);
static int test_isprint(int c);
static int test_isgraph(int c);
static int test_ispunct(int c);
static int test_isspace(int c);
static int test_isupper(int c);
static int test_isxdigit(int c);

/* our test-module */
sTestModule tModCtype = {
	"CType",
	&test_ctype
};

static void test_ctype(void) {
	int c;
	test_caseStart("Testing all characters");

	for(c = 0; c <= 0xFF; c++) {
		if(test_isalnum(c))
			test_assertTrue(isalnum(c));
		else
			test_assertFalse(isalnum(c));
		if(test_isalpha(c))
			test_assertTrue(isalpha(c));
		else
			test_assertFalse(isalpha(c));
		if(test_iscntrl(c))
			test_assertTrue(iscntrl(c));
		else
			test_assertFalse(iscntrl(c));
		if(test_isdigit(c))
			test_assertTrue(isdigit(c));
		else
			test_assertFalse(isdigit(c));
		if(test_islower(c))
			test_assertTrue(islower(c));
		else
			test_assertFalse(islower(c));
		if(test_isprint(c))
			test_assertTrue(isprint(c));
		else
			test_assertFalse(isprint(c));
		if(test_isgraph(c))
			test_assertTrue(isgraph(c));
		else
			test_assertFalse(isgraph(c));
		if(test_ispunct(c))
			test_assertTrue(ispunct(c));
		else
			test_assertFalse(ispunct(c));
		if(test_isspace(c))
			test_assertTrue(isspace(c));
		else
			test_assertFalse(isspace(c));
		if(test_isupper(c))
			test_assertTrue(isupper(c));
		else
			test_assertFalse(isupper(c));
		if(test_isxdigit(c))
			test_assertTrue(isxdigit(c));
		else
			test_assertFalse(isxdigit(c));
	}

	test_caseSucceeded();
}

static int test_isalnum(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}
static int test_isalpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static int test_iscntrl(int c) {
	return c < ' ' || c == 0x7F || c == 0xFF;
}
static int test_isdigit(int c) {
	return (c >= '0' && c <= '9');
}
static int test_islower(int c) {
	return (c >= 'a' && c <= 'z');
}
static int test_isprint(int c) {
	/* exclude 0xFF (=IO_EOF) */
	return (c >= ' ' && c < 0x7F) || (c >= 0x80 && c < 0xFF);
}
static int test_isgraph(int c) {
	/* exclude 0xFF (=IO_EOF) */
	return (c > ' ' && c < 0x7F) || (c >= 0x80 && c < 0xFF);
}
static int test_ispunct(int c) {
	return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') ||
		(c >= '{' && c <= '~');
}
static int test_isspace(int c) {
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}
static int test_isupper(int c) {
	return (c >= 'A' && c <= 'Z');
}
static int test_isxdigit(int c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
