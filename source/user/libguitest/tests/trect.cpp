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

#include <gui/graphics/rectangle.h>
#include <sys/common.h>
#include <sys/test.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gui;

static void test_rect(void);
static void test_contains(void);
static void test_splitEmpty(void);
static void test_splitHorVert(void);
static void test_splitCorner(void);
static void test_splitSide(void);
static void test_splitCenter(void);

static void assertRect(const Rectangle &recv,int x,int y,ushort width,ushort height);

/* our test-module */
sTestModule tModRect = {
	"Rectangle",
	&test_rect
};

static void test_rect(void) {
	test_contains();
	test_splitEmpty();
	test_splitHorVert();
	test_splitCorner();
	test_splitSide();
	test_splitCenter();
}

static void test_contains(void) {
	test_caseStart("Testing contains");

	{
		Rectangle r(Pos(0,0),Size(0,0));
		test_assertFalse(r.contains(0,0));
		test_assertFalse(r.contains(1,0));
		test_assertFalse(r.contains(0,1));
	}

	{
		Rectangle r(Pos(0,0),Size(1,0));
		test_assertFalse(r.contains(0,0));
		test_assertFalse(r.contains(1,0));
		test_assertFalse(r.contains(0,1));
	}

	{
		Rectangle r(Pos(0,0),Size(0,1));
		test_assertFalse(r.contains(0,0));
		test_assertFalse(r.contains(1,0));
		test_assertFalse(r.contains(0,1));
	}

	{
		Rectangle r(Pos(0,0),Size(1,1));
		test_assertTrue(r.contains(0,0));
		test_assertFalse(r.contains(1,0));
		test_assertFalse(r.contains(0,1));
	}

	{
		Rectangle r(Pos(0,0),Size(2,2));
		test_assertTrue(r.contains(0,0));
		test_assertTrue(r.contains(1,0));
		test_assertTrue(r.contains(0,1));
		test_assertTrue(r.contains(1,1));
	}

	test_caseSucceeded();
}

static void test_splitEmpty(void) {
	size_t oldFree;
	test_caseStart("Testing split with empty result");

	/* both empty */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,0),Size(0,0));
		Rectangle r2(Pos(0,0),Size(0,0));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rect to split empty */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,0),Size(0,0));
		Rectangle r2(Pos(0,0),Size(1,1));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (right) */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,0),Size(2,2));
		Rectangle r2(Pos(2,0),Size(2,2));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (left) */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(2,0),Size(2,2));
		Rectangle r2(Pos(0,0),Size(2,2));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (top) */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,2),Size(2,2));
		Rectangle r2(Pos(0,0),Size(2,2));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (bottom) */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,0),Size(2,2));
		Rectangle r2(Pos(0,2),Size(2,2));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	/* rect to split by contains the other */
	oldFree = heapspace();
	{
		Rectangle r1(Pos(2,2),Size(2,2));
		Rectangle r2(Pos(0,0),Size(6,6));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),0);
	}
	test_assertSize(heapspace(),oldFree);

	test_caseSucceeded();
}

static void test_splitHorVert(void) {
	size_t oldFree;

	test_caseStart("Testing split horizontally -> 1 rect");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(0,0),Size(12,5));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),1);
		assertRect(res[0],1,5,10,6);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split horizontally -> 2 rects");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(0,2),Size(12,5));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,1,10,1);
		assertRect(res[1],1,7,10,4);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split vertically -> 1 rect");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(0,1),Size(10,10));
		Rectangle r2(Pos(8,0),Size(3,12));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),1);
		assertRect(res[0],0,1,8,10);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split vertically -> 2 rects");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(4,0),Size(2,14));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,1,3,10);
		assertRect(res[1],6,1,5,10);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitCorner(void) {
	size_t oldFree;

	test_caseStart("Testing split @ top-left");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(0,0),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],4,1,7,3);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ top-right");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(8,0),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],1,1,7,3);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom-left");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(0,9),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,1,10,8);
		assertRect(res[1],4,9,7,2);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom-right");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(8,8),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),2);
		assertRect(res[0],1,1,10,7);
		assertRect(res[1],1,8,7,3);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitSide(void) {
	size_t oldFree;

	test_caseStart("Testing split @ top");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(4,0),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),3);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],1,1,3,3);
		assertRect(res[2],8,1,3,3);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ right");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(8,4),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),3);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],1,4,7,4);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(4,8),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),3);
		assertRect(res[0],1,1,10,7);
		assertRect(res[1],1,8,3,3);
		assertRect(res[2],8,8,3,3);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ left");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(0,4),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),3);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],4,4,7,4);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitCenter(void) {
	size_t oldFree;

	test_caseStart("Testing split @ center");
	oldFree = heapspace();
	{
		Rectangle r1(Pos(1,1),Size(10,10));
		Rectangle r2(Pos(4,4),Size(4,4));
		std::vector<Rectangle> res = substraction(r1,r2);
		test_assertSize(res.size(),4);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],1,4,3,4);
		assertRect(res[3],8,4,3,4);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void assertRect(const Rectangle &r,int x,int y,ushort width,ushort height) {
	test_assertInt(r.x(),x);
	test_assertInt(r.y(),y);
	test_assertUInt(r.width(),width);
	test_assertUInt(r.height(),height);
}
