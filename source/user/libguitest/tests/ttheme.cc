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

#include <esc/stream/istringstream.h>
#include <esc/stream/ostringstream.h>
#include <gui/theme/basetheme.h>
#include <sys/common.h>
#include <sys/test.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gui;

static void test_theme(void);

/* our test-module */
sTestModule tModTheme = {
	"Theme",
	&test_theme
};

static void test_theme(void) {
	BaseTheme t;
	for(size_t i = 0; i < 18; ++i)
		t.setColor(i,Color(i,i,i));
	t.setPadding(1);
	t.setTextPadding(2);

	esc::OStringStream os;
	t.serialize(os);

	esc::IStringStream is(os.str().c_str(),os.str().length());
	BaseTheme nt = BaseTheme::unserialize(is);

	for(size_t i = 0; i < 18; ++i) {
		test_assertInt(t.getColor(i).getRed(), nt.getColor(i).getRed());
		test_assertInt(t.getColor(i).getGreen(), nt.getColor(i).getGreen());
		test_assertInt(t.getColor(i).getBlue(), nt.getColor(i).getBlue());
		test_assertInt(t.getColor(i).getAlpha(), nt.getColor(i).getAlpha());
	}
	test_assertSize(t.getPadding(),nt.getPadding());
	test_assertSize(t.getTextPadding(),nt.getTextPadding());
}
