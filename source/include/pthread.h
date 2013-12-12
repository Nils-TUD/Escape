/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/thread.h>

typedef uint32_t pthread_key_t;
typedef int pthread_mutex_t;
typedef int pthread_t;
typedef long pthread_once_t;
typedef int pthread_mutexattr_t;

#ifdef __cplusplus
extern "C" {
#endif

int pthread_key_create(pthread_key_t * key,void (*f)(void*));
int pthread_key_delete(pthread_key_t key);
int pthread_cancel(pthread_t thread);
int pthread_once(pthread_once_t *control,void (*init)(void));
void* pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key,void* data);
int pthread_mutex_init(pthread_mutex_t *mutex,const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif
