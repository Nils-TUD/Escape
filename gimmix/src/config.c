/**
 * $Id: config.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "mmix/error.h"
#include "core/bus.h"
#include "cli/lang/symmap.h"
#include "config.h"

static bool testMode = false;
static bool userMode = false;
static const char *rom = NULL;
static const char *program = NULL;
static const char *diskImage = NULL;
static const char *outputFile = NULL;
static int termCount = TERM_DEF_COUNT;
static octa ramSize = RAM_DEF_SIZE;
static octa startAddr = DEF_START_ADDR;
static bool useBacktracing = false;
static const char *maps[MAX_MAPS];
static bool useDspKbd = false;

octa cfg_getStartAddr(void) {
	return startAddr;
}

void cfg_setStartAddr(octa addr) {
	if(addr & (sizeof(tetra) - 1))
		error("Address #%OX is not tetra-aligned",addr);
	startAddr = addr;
}

octa cfg_getRAMSize(void) {
	return ramSize;
}

void cfg_setRAMSize(octa size) {
	if(size == 0)
		error("The RAM-size may not be zero");
	if(size > RAM_END_ADDR)
		error("The maximum RAM-size is %Od bytes",RAM_END_ADDR);
	if(size & (sizeof(octa) - 1))
		error("The RAM-size %Od bytes is not octa-aligned",size);
	ramSize = size;
}

bool cfg_isTestMode(void) {
	return testMode;
}

void cfg_setTestMode(bool test) {
	testMode = test;
}

bool cfg_isUserMode(void) {
	return userMode;
}

void cfg_setUserMode(bool user) {
	userMode = user;
}

const char *cfg_getProgram(void) {
	return program;
}

void cfg_setProgram(const char *file) {
	program = file;
}

const char *cfg_getROM(void) {
	return rom;
}

void cfg_setROM(const char *file) {
	rom = file;
}

const char *cfg_getDiskImage(void) {
	return diskImage;
}

void cfg_setDiskImage(const char *file) {
	diskImage = file;
}

const char *cfg_getOutputFile(void) {
	return outputFile;
}

void cfg_setOutputFile(const char *file) {
	outputFile = file;
}

int cfg_getTermCount(void) {
	return termCount;
}

void cfg_setTermCount(int count) {
	if(count < 0 || count > TERM_MAX)
		error("Invalid number of terminals (0..%d allowed)",TERM_MAX);
	termCount = count;
}

bool cfg_useDspKbd(void) {
	return useDspKbd;
}

void cfg_setUseDspKbd(bool use) {
	useDspKbd = use;
}

bool cfg_useBacktracing(void) {
	return useBacktracing;
}

void cfg_setUseBacktracing(bool use) {
	useBacktracing = use;
}

const char **cfg_getMaps(void) {
	return maps;
}

void cfg_setMaps(const char **m) {
	memcpy(maps,m,MAX_MAPS * sizeof(char*));
}
