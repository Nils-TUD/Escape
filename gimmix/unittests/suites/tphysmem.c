/**
 * $Id: tphysmem.c 142 2011-03-11 12:28:00Z nasmussen $
 */

#include <stdlib.h>

#include "common.h"
#include "core/bus.h"
#include "tphysmem.h"

#define COUNT	20
#define MAGIC	0x1234567890ABCDEF

static void test_basic(void);
static void test_ascending(void);
static void test_descending(void);
static void test_mixed(void);

const sTestCase tPhysMem[] = {
	{"Basic write & read",test_basic},
	{"Write ascending & read",test_ascending},
	{"Write descending & read",test_descending},
	{"Write mixed & read",test_mixed},
	{NULL,0},
};

static void test_basic(void) {
	bus_write(0x00000000,MAGIC,false);
	test_assertOcta(bus_read(0x00000000,false),MAGIC);
	test_assertOcta(bus_read(0x00000008,false),0x0);

	bus_write(0x000FF000,MAGIC,false);
	test_assertOcta(bus_read(0x000FF000,false),MAGIC);
	test_assertOcta(bus_read(0x000FF008,false),0x0);
}

static void test_ascending(void) {
	octa addr = 0x000FF000;
	for(int i = 0; i < COUNT; i++) {
		bus_write(addr,MAGIC,false);
		addr += 0x1000;
	}
	addr = 0x000FF000;
	for(int i = 0; i < COUNT; i++) {
		test_assertOcta(bus_read(addr,false),MAGIC);
		addr += 0x1000;
	}
}

static void test_descending(void) {
	octa addr = 0x000FF000;
	for(int i = 0; i < COUNT; i++) {
		bus_write(addr,MAGIC,false);
		addr -= 0x1000;
	}
	addr = 0x000FF000;
	for(int i = 0; i < COUNT; i++) {
		test_assertOcta(bus_read(addr,false),MAGIC);
		addr -= 0x1000;
	}
}

static void test_mixed(void) {
	bus_write(0x00001000,MAGIC,false);
	bus_write(0x00006000,MAGIC,false);
	bus_write(0x00004000,MAGIC,false);
	bus_write(0x00009000,MAGIC,false);
	bus_write(0x00002000,MAGIC,false);
	bus_write(0x00005000,MAGIC,false);
	bus_write(0x00003000,MAGIC,false);
	bus_write(0x00008000,MAGIC,false);
	bus_write(0x00007000,MAGIC,false);
	for(octa addr = 0x00001000; addr <= 0x00009000; addr += 0x1000)
		test_assertOcta(bus_read(addr,false),MAGIC);
}
