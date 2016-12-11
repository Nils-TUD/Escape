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

#include "game.h"
#include "object.h"
#include "objlist.h"

void ObjList::add(Object *o) {
	Object *p = NULL;
	Object *obj = first;
	while(obj) {
		p = obj;
		obj = obj->_next;
	}
	if(p)
		p->_next = o;
	else
		first = o;
	o->_next = NULL;
}

int ObjList::tick(UI &ui) {
	bool removeO1;
	int scoreChg = 0;
	for(Object *pn = NULL, *o1 = first; o1 != NULL; ) {
		if(!o1->tick(ui)) {
			if(o1->type == Object::AIRPLANE)
				scoreChg += Game::SCORE_MISS;
			Object *nnext = o1->_next;
			delete o1;
			if(pn)
				pn->_next = nnext;
			else
				first = nnext;
			o1 = nnext;
		}
		else {
			pn = o1;
			o1 = o1->_next;
		}
	}

	for(Object *pn = NULL, *o1 = first; o1 != NULL; ) {
		removeO1 = false;
		for(Object *pm = NULL, *o2 = first; o2 != NULL; pm = o2, o2 = o2->_next) {
			if(o1 != o2) {
				if(o1->collide(o2)) {
					scoreChg += Game::SCORE_HIT;
					if(!o1->explode()) {
						removeO1 = true;
						delete o1;
					}
					if(!o2->explode()) {
						if(pm)
							pm->_next = o2->_next;
						else
							first = o2->_next;
						delete o2;
					}
					break;
				}
			}
		}

		/* collition? */
		if(removeO1) {
			Object *nnext = o1->_next;
			if(pn)
				pn->_next = o1->_next;
			else
				first = o1->_next;
			o1 = nnext;
		}
		else {
			pn = o1;
			o1 = o1->_next;
		}
	}
	return scoreChg;
}
