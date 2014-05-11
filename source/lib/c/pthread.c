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

#include <esc/common.h>
#include <esc/atomic.h>
#include <esc/sync.h>
#include <esc/thread.h>
#include <pthread.h>

#define MAX_LOCKS   4

static pthread_key_t next_key = 0x12345678;
static long lockCount = 0;
static tUserSem usems[MAX_LOCKS];

int pthread_key_create(pthread_key_t* key,A_UNUSED void (*func)(void*)) {
    *key = next_key++;
    return 0;
}

int pthread_key_delete(A_UNUSED pthread_key_t key) {
    return 0;
}

int pthread_cancel(A_UNUSED pthread_t id) {
    return 0;
}

int pthread_once(pthread_once_t *control,void (*init)(void)) {
    if(atomic_cmpnswap(control,1,2))
        (*init)();
    return 0;
}

void* pthread_getspecific(pthread_key_t key) {
    return getThreadVal(key);
}

int pthread_setspecific(pthread_key_t key,void* data) {
    return setThreadVal(key,data) ? 0 : -1;
}

int pthread_mutex_init(pthread_mutex_t *mutex,A_UNUSED const pthread_mutexattr_t *attr) {
    int id = atomic_add(&lockCount,+1);
    *mutex = id;
    return usemcrt(usems + id,1);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    usemdown(usems + *mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    usemup(usems + *mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    usemdestr(usems + *mutex);
    return 0;
}
