/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <assert.h>
#include "object.h"
#include "objlist.h"
#include "game.h"

static sObject *first;

void objlist_add(sObject *o) {
	sObject *p = NULL;
	sObject *obj = first;
	while(obj) {
		p = obj;
		obj = obj->next;
	}
	if(p)
		p->next = o;
	else
		first = o;
	o->next = NULL;
}

sObject *objlist_get(void) {
	return first;
}

int objlist_tick(void) {
	bool removeO1;
	int scoreChg = 0;
	for(sObject *pn = NULL, *o1 = first; o1 != NULL; ) {
		if(!obj_tick(o1)) {
			if(o1->type == TYPE_AIRPLANE)
				scoreChg += SCORE_MISS;
			sObject *nnext = o1->next;
			obj_destroy(o1);
			if(pn)
				pn->next = nnext;
			else
				first = nnext;
			o1 = nnext;
		}
		else {
			pn = o1;
			o1 = o1->next;
		}
	}

	for(sObject *pn = NULL, *o1 = first; o1 != NULL; ) {
		removeO1 = false;
		for(sObject *pm = NULL, *o2 = first; o2 != NULL; pm = o2, o2 = o2->next) {
			if(o1 != o2) {
				if(obj_collide(o1,o2)) {
					scoreChg += SCORE_HIT;
					if(!obj_explode(o1)) {
						removeO1 = true;
						obj_destroy(o1);
					}
					if(!obj_explode(o2)) {
						if(pm)
							pm->next = o2->next;
						else
							first = o2->next;
						obj_destroy(o2);
					}
					break;
				}
			}
		}

		/* collition? */
		if(removeO1) {
			sObject *nnext = o1->next;
			if(pn)
				pn->next = o1->next;
			else
				first = o1->next;
			o1 = nnext;
		}
		else {
			pn = o1;
			o1 = o1->next;
		}
	}
	return scoreChg;
}
