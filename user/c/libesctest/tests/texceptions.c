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

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/outofmemory.h>
#include <test.h>
#include <errors.h>
#include "texceptions.h"

/* forward declarations */
static void test_exc(void);
static void test_trycatch(void);
static void test_tryfinally(void);
static void test_trythrow(void);
static void test_tryinfinally(void);
static void test_multiplecatch(void);
static void test_trynested(void);

/* our test-module */
sTestModule tModExc = {
	"Exceptions",
	&test_exc
};

static void test_exc(void) {
	test_trycatch();
	test_tryfinally();
	test_trythrow();
	test_tryinfinally();
	test_multiplecatch();
	test_trynested();
}

static void test_doThrow(void) {
	THROW(IOException,ERR_EOF);
}

static void test_trycatch(void) {
	u32 i,j,before;
	test_caseStart("Testing try & catch without throw");

	i = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	ENDCATCH
	test_assertUInt(i,1);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();

	test_caseStart("Testing try & catch with throw");

	i = 0;
	j = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		test_doThrow();
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(j,0);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}

static void test_tryfinally(void) {
	u32 i,j,before;
	test_caseStart("Testing try & finally without throw");

	i = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
	}
	FINALLY {
		i = 2;
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();

	test_caseStart("Testing try & catch & finally with throw");

	i = 0;
	j = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		test_doThrow();
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	FINALLY {
		i = 3;
	}
	ENDCATCH
	test_assertUInt(i,3);
	test_assertUInt(j,0);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}

static void test_trythrow(void) {
	u32 i,j,before;
	test_caseStart("Testing try & throw");

	i = 0;
	j = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		THROW(IOException,ERR_EOF);
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(j,0);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();

	test_caseStart("Testing try & finally & throw");

	i = 0;
	j = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		THROW(IOException,ERR_EOF);
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	FINALLY {
		i = 3;
	}
	ENDCATCH
	test_assertUInt(i,3);
	test_assertUInt(j,0);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}

static void test_tryinfinally(void) {
	u32 i,j,k,before;
	test_caseStart("Testing try & catch in finally");

	i = 0;
	j = 0;
	k = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		THROW(IOException,-1);
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	FINALLY {
		TRY {
			k = 1;
			test_doThrow();
			j = 1;
		}
		CATCH(IOException,e) {
			k = 2;
		}
		FINALLY {
			k = 3;
		}
		ENDCATCH
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(j,0);
	test_assertUInt(k,3);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}

static void test_multiplecatch(void) {
	u32 i,j,k,before;
	test_caseStart("Testing try & multiple catch clauses");

	i = 0;
	j = 0;
	k = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		test_doThrow();
		j = 1;
	}
	CATCH(OutOfMemoryException,e) {
		j = 2;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	FINALLY {
		k = 1;
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(j,0);
	test_assertUInt(k,1);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}

static void test_trynested1(void) {
	TRY {
		test_doThrow();
	}
	CATCH(IOException,e) {
		RETHROW(e);
	}
	ENDCATCH
}

static void test_trynested(void) {
	u32 i,j,k,before;
	test_caseStart("Testing nested try & rethrow");

	i = 0;
	j = 0;
	k = 0;
	before = heap_getFreeSpace();
	TRY {
		i = 1;
		test_trynested1();
		j = 1;
	}
	CATCH(IOException,e) {
		i = 2;
	}
	FINALLY {
		k = 1;
	}
	ENDCATCH
	test_assertUInt(i,2);
	test_assertUInt(j,0);
	test_assertUInt(k,1);
	test_assertUInt(heap_getFreeSpace(),before);

	test_caseSucceded();
}
