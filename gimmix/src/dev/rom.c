/**
 * $Id: rom.c 218 2011-06-04 15:46:40Z nasmussen $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "common.h"
#include "dev/rom.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "mmix/mem.h"
#include "exception.h"
#include "config.h"
#include "sim.h"

static void rom_reset(void);
static void rom_shutdown(void);
static octa rom_read(octa addr,bool sideEffects);
static void rom_write(octa addr,octa data);

static sDevice romDev;
static byte *rom;
static FILE *romImage;
static size_t romSize;

void rom_init(void) {
	// allocate ROM
	rom = mem_alloc(ROM_SIZE);
	if(rom == NULL)
		sim_error("cannot allocate ROM");

	// possibly load ROM image
	const char *romImageName = cfg_getROM();
	if(romImageName == NULL)
		romImage = NULL;
	else {
		// plug in ROM
		romImage = fopen(romImageName,"rb");
		if(romImage == NULL)
			sim_error("cannot open ROM image '%s'",romImageName);
		fseek(romImage,0,SEEK_END);
		romSize = ftell(romImage);
		if(romSize > ROM_SIZE)
			sim_error("ROM image too big");

		// register device
		romDev.name = "ROM";
		romDev.startAddr = ROM_START_ADDR;
		romDev.endAddr = ROM_START_ADDR + romSize - 1;
		romDev.irqMask = 0;
		romDev.reset = rom_reset;
		romDev.read = rom_read;
		romDev.write = rom_write;
		romDev.shutdown = rom_shutdown;
		bus_register(&romDev);
	}
	rom_reset();
}

static void rom_reset(void) {
	for(size_t i = 0; i < ROM_SIZE; i++)
		rom[i] = rand();
	if(romImage != NULL) {
		fseek(romImage,0,SEEK_SET);
		if(fread(rom,romSize,1,romImage) != 1)
			sim_error("cannot read ROM image file");
		for(size_t i = romSize; i < ROM_SIZE; i++)
			rom[i] = 0xFF;
	}
}

static void rom_shutdown(void) {
	mem_free(rom);
}

static octa rom_read(octa addr,bool sideEffects) {
	UNUSED(sideEffects);
	if(addr >= ROM_START_ADDR && addr <= ROM_START_ADDR + ROM_SIZE - sizeof(octa)) {
		addr -= ROM_START_ADDR;
		return ((octa)rom[addr + 0]) << 56 |
				((octa)rom[addr + 1]) << 48 |
				((octa)rom[addr + 2]) << 40 |
				((octa)rom[addr + 3]) << 32 |
				((octa)rom[addr + 4]) << 24 |
				((octa)rom[addr + 5]) << 16 |
				((octa)rom[addr + 6]) << 8 |
				((octa)rom[addr + 7]) << 0;
	}

	ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
	return 0;
}

static void rom_write(octa addr,octa data) {
	UNUSED(addr);
	UNUSED(data);
	// no writing here
	ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);
}
