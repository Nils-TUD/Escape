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
#include <assert.h>
#include <esc/sllist.h>
#include "object.h"
#include "objlist.h"
#include "game.h"

static sSLList *objects;

void objlist_create(void) {
	objects = sll_create();
	assert(objects != NULL);
}

void objlist_add(sObject *o) {
	assert(sll_append(objects,o));
}

sSLList *objlist_get(void) {
	return objects;
}

s32 objlist_tick(void) {
	sSLNode *n,*pn,*nnext,*m,*pm;
	sObject *o1,*o2;
	bool removeO1;
	s32 scoreChg = 0;
	pn = NULL;
	for(n = sll_begin(objects); n != NULL; ) {
		o1 = (sObject*)n->data;
		if(!obj_tick(o1)) {
			if(o1->type == TYPE_AIRPLANE)
				scoreChg += SCORE_MISS;
			nnext = n->next;
			obj_destroy(o1);
			sll_removeNode(objects,n,pn);
			n = nnext;
		}
		else {
			pn = n;
			n = n->next;
		}
	}

	pn = NULL;
	for(n = sll_begin(objects); n != NULL; ) {
		pm = NULL;
		removeO1 = false;
		for(m = sll_begin(objects); m != NULL; pm = m, m = m->next) {
			if(n != m) {
				o1 = (sObject*)n->data;
				o2 = (sObject*)m->data;
				if(obj_collide(o1,o2)) {
					scoreChg += SCORE_HIT;
					if(!obj_explode(o1)) {
						removeO1 = true;
						obj_destroy(o1);
					}
					if(!obj_explode(o2)) {
						obj_destroy(o2);
						sll_removeNode(objects,m,pm);
					}
					break;
				}
			}
		}

		/* collition? */
		if(removeO1) {
			nnext = n->next;
			sll_removeNode(objects,n,pn);
			n = nnext;
		}
		else {
			pn = n;
			n = n->next;
		}
	}
	return scoreChg;
}
