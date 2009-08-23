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
#include <esc/heap.h>
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include <stdlib.h>

#define LOCK_ARRAY	0x1
#define TEST_COUNT	100

typedef struct {
	void **ptr;
	u32 dataSize;
	u32 num;
	u32 count;
} sArray;

static int myThread(void);
static void treadDir(const char *path);
static sArray *create(u32 dataSize,u32 num);
static void add(sArray *a,void *e);
static void print(sArray *a);

tULock alock = 0;
sArray *array;

int main(void) {
	array = create(sizeof(u32),10);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	startThread(myThread);
	return EXIT_SUCCESS;
}

static int myThread(void) {
	const char *folders[] = {
		"file:/","file:/bin","system:"
	};
	while(1) {
		treadDir(folders[gettid() % ARRAY_SIZE(folders)]);
		/*printf("I am thread %d\n",gettid());*/
	}
	/*u32 i;
	u32 *ptrs[TEST_COUNT];
	for(i = 0; i < TEST_COUNT; i++) {
		dbg_startTimer();
		ptrs[i] = malloc(sizeof(u32));
		dbg_stopTimer("malloc");
	}
	for(i = 0; i < TEST_COUNT; i++) {
		dbg_startTimer();
		free(ptrs[i]);
		dbg_stopTimer("free");
	}*/
	/*add(a,(void*)10);
	add(a,(void*)11);
	add(a,(void*)12);
	print(a);*/
	return 0;
}

static void treadDir(const char *path) {
	tTid tid = gettid();
	tFD dir = opendir(path);
	if(dir < 0) {
		printe("Unable to open '%s'",path);
		return;
	}

	printf("[%d] opening '%s'\n",tid,path);
	u32 i;
	sDirEntry e;
	while(readdir(&e,dir)) {
		if(i++ % 3 == 0)
			yield();
		printf("[%d] '%s'\n",tid,e.name);
	}
	printf("[%d] done\n",tid);

	closedir(dir);
}

static sArray *create(u32 dataSize,u32 num) {
	sArray *a = (sArray*)malloc(sizeof(sArray));
	a->ptr = (void**)malloc(dataSize * num);
	a->dataSize = dataSize;
	a->num = num;
	a->count = 0;
	return a;
}

static void add(sArray *a,void *e) {
	locku(&alock);
	if(a->count >= a->num) {
		a->num *= 2;
		a->ptr = (void**)realloc(a->ptr,a->dataSize * a->num);
	}
	a->ptr[a->count] = e;
	a->count++;
	unlocku(&alock);
}

static void print(sArray *a) {
	locku(&alock);
	u32 i;
	printf("Thread %d: Array[size=%d, elements=%d]\n",gettid(),a->num,a->count);
	for(i = 0; i < a->count; i++)
		printf("\t%d: %x\n",i,a->ptr[i]);
	unlocku(&alock);
}
