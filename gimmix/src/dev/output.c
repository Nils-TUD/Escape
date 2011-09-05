/**
 * $Id: output.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "dev/output.h"
#include "core/bus.h"
#include "config.h"
#include "sim.h"

static void output_reset(void);
static void output_shutdown(void);
static octa output_read(octa addr,bool sideEffects);
static void output_write(octa addr,octa data);

static sDevice outputDev;
static FILE *outputFile;

void output_init(void) {
	const char *outputFileName = cfg_getOutputFile();
	if(outputFileName == NULL)
		outputFile = NULL;
	else {
		// try to install output device
		outputFile = fopen(outputFileName,"wb");
		if(outputFile == NULL)
			sim_error("cannot open output device file '%s'",outputFileName);
		setvbuf(outputFile,NULL,_IONBF,0);

		// register device
		outputDev.name = "Output";
		outputDev.startAddr = OUTPUT_START_ADDR;
		outputDev.endAddr = OUTPUT_START_ADDR + sizeof(octa) - 1;
		outputDev.irqMask = 0;
		outputDev.reset = output_reset;
		outputDev.read = output_read;
		outputDev.write = output_write;
		outputDev.shutdown = output_shutdown;
		bus_register(&outputDev);
	}
	output_reset();
}

static void output_reset(void) {
	// do nothing
}

static void output_shutdown(void) {
	// output device not installed
	if(outputFile == NULL)
		return;

	fclose(outputFile);
}

static octa output_read(octa addr,bool sideEffects) {
	UNUSED(addr);
	UNUSED(sideEffects);
	// output device not installed
	if(outputFile == NULL)
		sim_error("output device not installed");

	// output device always returns 0 on read
	return 0;
}

static void output_write(octa addr,octa data) {
	UNUSED(addr);
	// output device not installed
	if(outputFile == NULL)
		sim_error("output device not installed");

	char c = data;
	if(fwrite(&c,1,1,outputFile) != 1)
		sim_error("write error on output device");
}
