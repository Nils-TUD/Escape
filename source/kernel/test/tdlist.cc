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

#include <common.h>
#include <col/dlist.h>
#include <esc/test.h>

static void test_dlist();

sTestModule tModDList = {
    "DList",test_dlist
};

static void test_dlist() {
    DListItem e1,e2,e3;
    DList<DListItem>::iterator it;
    DList<DListItem> l;
    test_assertSize(l.length(),static_cast<size_t>(0));
    test_assertTrue(l.begin() == l.end());

    l.append(&e1);
    l.append(&e2);
    l.append(&e3);
    test_assertSize(l.length(),static_cast<size_t>(3));
    it = l.begin();
    test_assertTrue(&*it == &e1);
    ++it;
    test_assertTrue(&*it == &e2);
    ++it;
    test_assertTrue(&*it == &e3);
    ++it;
    test_assertTrue(it == l.end());

    l.remove(&e2);
    test_assertSize(l.length(),static_cast<size_t>(2));
    it = l.begin();
    test_assertTrue(&*it == &e1);
    ++it;
    test_assertTrue(&*it == &e3);
    ++it;
    test_assertTrue(it == l.end());

    l.remove(&e3);
    test_assertSize(l.length(),static_cast<size_t>(1));
    it = l.begin();
    test_assertTrue(&*it == &e1);
    ++it;
    test_assertTrue(it == l.end());

    l.append(&e3);
    test_assertSize(l.length(),static_cast<size_t>(2));
    it = l.begin();
    test_assertTrue(&*it == &e1);
    ++it;
    test_assertTrue(&*it == &e3);
    ++it;
    test_assertTrue(it == l.end());

    l.remove(&e1);
    l.remove(&e3);
    test_assertSize(l.length(),static_cast<size_t>(0));
    test_assertTrue(l.begin() == l.end());

    l.append(&e2);
    test_assertSize(l.length(),static_cast<size_t>(1));
    it = l.begin();
    test_assertTrue(&*it == &e2);
    ++it;
    test_assertTrue(it == l.end());
}
