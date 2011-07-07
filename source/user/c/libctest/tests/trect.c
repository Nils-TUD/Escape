/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/test.h>
#include <esc/rect.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "trect.h"

static void test_rect(void);
static void test_contains(void);
static void test_splitEmpty(void);
static void test_splitHorVert(void);
static void test_splitCorner(void);
static void test_splitSide(void);
static void test_splitCenter(void);

static void assertRect(sRectangle *recv,int x,int y,ushort width,ushort height);
static sRectangle create(int x,int y,ushort width,ushort height);

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
		sRectangle r = create(0,0,0,0);
		test_assertFalse(rectContains(&r,0,0));
		test_assertFalse(rectContains(&r,1,0));
		test_assertFalse(rectContains(&r,0,1));
	}

	{
		sRectangle r = create(0,0,1,0);
		test_assertFalse(rectContains(&r,0,0));
		test_assertFalse(rectContains(&r,1,0));
		test_assertFalse(rectContains(&r,0,1));
	}

	{
		sRectangle r = create(0,0,0,1);
		test_assertFalse(rectContains(&r,0,0));
		test_assertFalse(rectContains(&r,1,0));
		test_assertFalse(rectContains(&r,0,1));
	}

	{
		sRectangle r = create(0,0,1,1);
		test_assertTrue(rectContains(&r,0,0));
		test_assertFalse(rectContains(&r,1,0));
		test_assertFalse(rectContains(&r,0,1));
	}

	{
		sRectangle r = create(0,0,2,2);
		test_assertTrue(rectContains(&r,0,0));
		test_assertTrue(rectContains(&r,1,0));
		test_assertTrue(rectContains(&r,0,1));
		test_assertTrue(rectContains(&r,1,1));
	}

	test_caseSucceeded();
}

static void test_splitEmpty(void) {
	size_t oldFree;
	test_caseStart("Testing split with empty result");

	/* both empty */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,0,0,0);
		sRectangle r2 = create(0,0,0,0);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rect to split empty */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,0,0,0);
		sRectangle r2 = create(0,0,1,1);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (right) */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,0,2,2);
		sRectangle r2 = create(2,0,2,2);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (left) */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(2,0,2,2);
		sRectangle r2 = create(0,0,2,2);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (top) */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,2,2,2);
		sRectangle r2 = create(0,0,2,2);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rects do not overlap (bottom) */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,0,2,2);
		sRectangle r2 = create(0,2,2,2);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	/* rect to split by contains the other */
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(2,2,2,2);
		sRectangle r2 = create(0,0,6,6);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertPtr(res,NULL);
		test_assertSize(count,0);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);

	test_caseSucceeded();
}

static void test_splitHorVert(void) {
	size_t oldFree;

	test_caseStart("Testing split horizontally -> 1 rect");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(0,0,12,5);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,1);
		assertRect(res[0],1,5,10,6);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split horizontally -> 2 rects");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(0,2,12,5);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,1,10,1);
		assertRect(res[1],1,7,10,4);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split vertically -> 1 rect");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(0,1,10,10);
		sRectangle r2 = create(8,0,3,12);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,1);
		assertRect(res[0],0,1,8,10);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split vertically -> 2 rects");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(4,0,2,14);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,1,3,10);
		assertRect(res[1],6,1,5,10);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitCorner(void) {
	size_t oldFree;

	test_caseStart("Testing split @ top-left");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(0,0,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],4,1,7,3);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ top-right");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(8,0,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],1,1,7,3);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom-left");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(0,9,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,1,10,8);
		assertRect(res[1],4,9,7,2);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom-right");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(8,8,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,2);
		assertRect(res[0],1,1,10,7);
		assertRect(res[1],1,8,7,3);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitSide(void) {
	size_t oldFree;

	test_caseStart("Testing split @ top");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(4,0,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,3);
		assertRect(res[0],1,4,10,7);
		assertRect(res[1],1,1,3,3);
		assertRect(res[2],8,1,3,3);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ right");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(8,4,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,3);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],1,4,7,4);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ bottom");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(4,8,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,3);
		assertRect(res[0],1,1,10,7);
		assertRect(res[1],1,8,3,3);
		assertRect(res[2],8,8,3,3);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();

	test_caseStart("Testing split @ left");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(0,4,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,3);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],4,4,7,4);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_splitCenter(void) {
	size_t oldFree;

	test_caseStart("Testing split @ center");
	oldFree = heapspace();
	{
		size_t count;
		sRectangle r1 = create(1,1,10,10);
		sRectangle r2 = create(4,4,4,4);
		sRectangle **res = rectSplit(&r1,&r2,&count);
		test_assertSize(count,4);
		assertRect(res[0],1,1,10,3);
		assertRect(res[1],1,8,10,3);
		assertRect(res[2],1,4,3,4);
		assertRect(res[3],8,4,3,4);
		rectFree(res,count);
	}
	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void assertRect(sRectangle *recv,int x,int y,ushort width,ushort height) {
	test_assertInt(recv->x,x);
	test_assertInt(recv->y,y);
	test_assertUInt(recv->width,width);
	test_assertUInt(recv->height,height);
}

static sRectangle create(int x,int y,ushort width,ushort height) {
	sRectangle r;
	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;
	return r;
}
