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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/kevent.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <errors.h>

/* a kevent */
typedef struct {
	u32 event;
	fKEvCallback callback;
} sKEvent;

/* our listeners */
static sSLList *events = NULL;

void kev_init(void) {
	events = sll_create();
	if(events == NULL)
		util_panic("Not enough mem for kevents");
}

s32 kev_add(u32 event,fKEvCallback callback) {
	sKEvent *ev = (sKEvent*)kheap_alloc(sizeof(sKEvent));
	if(ev == NULL)
		return ERR_NOT_ENOUGH_MEM;
	ev->event = event;
	ev->callback = callback;
	if(!sll_append(events,ev)) {
		kheap_free(ev);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

u32 kev_notify(u32 event,u32 param) {
	sKEvent *ev;
	sSLNode *n,*p,*t;
	u32 x = 0;
	p = NULL;
	for(n = sll_begin(events); n != NULL; ) {
		ev = (sKEvent*)n->data;
		if(ev->event == event) {
			/* notify and remove */
			t = n->next;
			ev->callback(param);
			kheap_free(ev);
			sll_removeNode(events,n,p);
			x++;
			n = t;
		}
		else {
			p = n;
			n = n->next;
		}
	}
	return x;
}

#if DEBUGGING

void kev_print(void) {
	sKEvent *ev;
	sSLNode *n;
	sSymbol *sym;
	vid_printf("KEvents:\n");
	for(n = sll_begin(events); n != NULL; n = n->next) {
		ev = (sKEvent*)n->data;
		sym = ksym_getSymbolAt((u32)ev->callback);
		vid_printf("\t%d - %s (0x%x)\n",ev->event,sym ? sym->funcName : "Unknown",ev->callback);
	}
}

#endif
