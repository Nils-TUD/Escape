/*
 * Placed into the Public Domain
 * written by Walter Bright, Digital Mars
 * www.digitalmars.com
 */

#include <esc/common.h>
#include <esc/lock.h>
#include <esc/proc.h>

/******************************************
 * Enter/exit critical section.
 */

/* We don't initialize critical sections unless we actually need them.
 * So keep a linked list of the ones we do use, and in the static destructor
 * code, walk the list and release them.
 */

typedef struct D_CRITICAL_SECTION {
	struct D_CRITICAL_SECTION *next;
	tULock cs;
} D_CRITICAL_SECTION;

static D_CRITICAL_SECTION *dcs_list;
static D_CRITICAL_SECTION critical_section;

void _d_criticalenter(D_CRITICAL_SECTION *dcs);
void _d_criticalexit(D_CRITICAL_SECTION *dcs);
void _STI_critical_init(void);
void _STD_critical_term(void*);

void _d_criticalenter(D_CRITICAL_SECTION *dcs) {
	if(!dcs_list) {
		_STI_critical_init();
		atexit(_STD_critical_term);
	}

	if(!dcs->next) {
		locku(&critical_section.cs);
		/* if, in the meantime, another thread didn't set it */
		if(!dcs->next) {
			dcs->next = dcs_list;
			dcs_list = dcs;
			dcs->cs = 0;
		}
		unlocku(&critical_section.cs);
	}
	locku(&dcs->cs);
}

void _d_criticalexit(D_CRITICAL_SECTION *dcs) {
	unlocku(&dcs->cs);
}

void _STI_critical_init(void) {
	if(!dcs_list)
		dcs_list = &critical_section;
}

void _STD_critical_term(void *dummy) {
	UNUSED(dummy);
	if(dcs_list)
		dcs_list = NULL;
}
