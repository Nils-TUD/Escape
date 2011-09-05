/**
 * $Id: sim.c 202 2011-05-10 10:49:04Z nasmussen $
 */

#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>

#include "common.h"
#include "core/bus.h"
#include "core/cache.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/mmu.h"
#include "core/register.h"
#include "core/tc.h"
#include "mmix/error.h"
#include "config.h"
#include "event.h"
#include "sim.h"

typedef void (*fModInit)(void);
typedef void (*fModReset)(void);
typedef void (*fModShutdown)(void);

typedef struct {
	const char *name;
	fModInit init;
	fModReset reset;
	fModShutdown shutdown;
} sModule;

static void sim_setup(void);

static const sModule modules[] = {
	{"Bus",			bus_init,		bus_reset,		bus_shutdown},
	{"Cache",		cache_init,		cache_reset,	cache_shutdown},
	{"CPU",			cpu_init,		cpu_reset,		NULL},
	{"Decoder",		NULL,			NULL,			NULL},
	{"MMU",			NULL,			NULL,			NULL},
	{"Register",	reg_init,		reg_reset,		NULL},
	{"TC",			tc_init,		tc_reset,		tc_shutdown},
};

static void sim_sigIntrpt(int sig) {
	UNUSED(sig);
	ev_fire(EV_USER_INT);
	if(signal(SIGINT,sim_sigIntrpt) == SIG_ERR)
		error("Unable to re-announce SIGINT-handler");
}

static void sim_sigTerm(int sig) {
	UNUSED(sig);
	sim_shutdown();
	exit(EXIT_FAILURE);
}

void sim_init(void) {
	if(signal(SIGINT,sim_sigIntrpt) == SIG_ERR)
		error("Unable to announce SIGINT-handler");
	if(signal(SIGTERM,sim_sigTerm) == SIG_ERR)
		error("Unable to announce SIGTERM-handler");

	for(size_t i = 0; i < ARRAY_SIZE(modules); i++) {
		if(modules[i].init)
			modules[i].init();
	}
	sim_setup();
}

void sim_reset(void) {
	for(size_t i = 0; i < ARRAY_SIZE(modules); i++) {
		if(modules[i].reset)
			modules[i].reset();
	}
	sim_setup();
}

void sim_shutdown(void) {
	for(size_t i = 0; i < ARRAY_SIZE(modules); i++) {
		if(modules[i].shutdown)
			modules[i].shutdown();
	}
}

void sim_error(const char *msg,...) {
	sim_shutdown();
	va_list ap;
	va_start(ap,msg);
	verror(msg,ap);
	// not reached
	va_end(ap);
}

static void sim_setup(void) {
	cpu_setPC(cfg_getStartAddr());

	// setup user-environment
	if(cfg_isUserMode()) {
		reg_setSpecial(rS,0x6000000000000000);
		reg_setSpecial(rO,0x6000000000000000);
		reg_setSpecial(rK,0xFFFFFFFFFFFFFFFF);
		reg_setSpecial(rT,0x8000000500000000);
		reg_setSpecial(rTT,0x8000000600000000);
		// segmentsizes: 3,6,9,12; pageSize=2^32; r=0x400000000; n=0
		reg_setSpecial(rV,0x369c200400000000);
		// one PTE for each segment
		bus_write(0x400000000,0x0000000000000007,false);
		bus_write(0x400006000,0x0000000100000006,false);
		bus_write(0x40000C000,0x0000000200000006,false);
		bus_write(0x400012000,0x0000000300000006,false);
	}
}
