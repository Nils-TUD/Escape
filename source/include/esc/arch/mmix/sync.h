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

#pragma once

#include <esc/common.h>
#include <esc/atomic.h>

static inline int usemcrt(tUserSem *sem,long val) {
	sem->value = val;
	sem->sem = semcrt(0);
	return sem->sem;
}

static inline void usemdown(tUserSem *sem) {
	/* 1 means free, <= 0 means taken */
	if(atomic_add(&sem->value,-1) <= 0)
		semdown(sem->sem);
}

static inline void usemup(tUserSem *sem) {
	if(atomic_add(&sem->value,+1) < 0)
		semup(sem->sem);
}

static inline void usemdestr(tUserSem *sem) {
	semdestr(sem->sem);
}
