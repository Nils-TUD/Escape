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
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/env.h>
#include <esc/fileio.h>
#include <esc/lock.h>
#include <stdlib.h>

/* macro to get a pointer to element <elNo> from <array> with a size of <elSize> per element */
#define QSORT_AGET(array,elSize,elNo) (((char*)(array)) + (elSize) * (elNo))

/**
 * Quicksort-implementation
 * source: http://de.wikipedia.org/wiki/Quicksort
 */
static void _qsort(void *base,size_t size,fCompare cmp,int left,int right);
static int divide(void *base,size_t size,fCompare cmp,int left,int right);

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static tULock randLock = 0;
static unsigned int randm = RAND_MAX;
static unsigned int randa = 1103515245;
static unsigned int randc = 12345;
static unsigned int lastRand = 0;

int rand(void) {
	locku(&randLock);
	lastRand = (randa * lastRand + randc) % randm;
	unlocku(&randLock);
	return lastRand;
}

void srand(unsigned int seed) {
	lastRand = seed;
}

void abort(void) {
	exit(EXIT_FAILURE);
}

char *getenv(const char *name) {
	/* TODO */
	static char val[MAX_PATH_LEN + 1];
	if(getEnv(val,MAX_PATH_LEN + 1,name))
		return val;
	return NULL;
}

int system(const char *cmd) {
	s32 child;
	/* check wether we have a shell */
	if(cmd == NULL) {
		tFD fd = open("/bin/shell",IO_READ);
		if(fd >= 0) {
			close(fd);
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	child = fork();
	if(child == 0) {
		const char *args[] = {"/bin/shell","-e",NULL,NULL};
		args[2] = cmd;
		exec(args[0],args);

		/* if we're here there is something wrong */
		error("Exec of '%s' failed",args[0]);
	}
	else if(child < 0)
		error("Fork failed");

	/* TODO we need the return-value of the child.. */
	wait(EV_CHILD_DIED);
	return EXIT_SUCCESS;
}

void *bsearch(const void *key,const void *base,size_t num,size_t size,fCompare cmp) {
	unsigned long int from = 0;
	unsigned long int to = num - 1;
	unsigned long int m;
	void *val;
	long int res;

	while(from <= to) {
		m = (from + to) / 2;
		val = (void*)((unsigned long int)base + m * size);

		/* compare */
		res = cmp(val,key);

		/* have we found it? */
		if(res == 0)
			return val;

		/* where to continue? */
		if(res > 0)
			to = m - 1;
		else
			from = m + 1;
	}
	return NULL;
}

void qsort(void *base,size_t nmemb,size_t size,fCompare cmp) {
	_qsort(base,size,cmp,0,nmemb - 1);
}

static void _qsort(void *base,size_t size,fCompare cmp,int left,int right) {
	/* TODO someday we should provide a better implementation which uses another sort-algo
	 * for small arrays, don't uses recursion and so on */
	if(left < right) {
		int i = divide(base,size,cmp,left,right);
		_qsort(base,size,cmp,left,i - 1);
		_qsort(base,size,cmp,i + 1,right);
	}
}

static int divide(void *base,size_t size,fCompare cmp,int left,int right) {
	char *pleft = (char*)base + left * size;
	char *piv = (char*)base + right * size;
	char *i = pleft;
	char *j = piv - size;
	do {
		/* right until the element is > piv */
		while(cmp(i,piv) <= 0 && i < piv)
			i += size;
		/* left until the element is < piv */
		while(cmp(j,piv) >= 0 && j > pleft)
			j -= size;

		/* swap */
		if(i < j) {
			memswp(i,j,size);
			i += size;
			j -= size;
		}
	}
	while(i < j);

	/* swap piv with element i */
	if(cmp(i,piv) > 0)
		memswp(i,piv,size);
	return (u32)(i - (char*)base) / size;
}

int abs(int n) {
	return n < 0 ? -n : n;
}

long int labs(long int n) {
	return n < 0 ? -n : n;
}

div_t div(int numerator,int denominator) {
	div_t res;
	res.quot = numerator / denominator;
	res.rem = numerator % denominator;
	return res;
}

ldiv_t ldiv(long numerator,long denominator) {
	ldiv_t res;
	res.quot = numerator / denominator;
	res.rem = numerator % denominator;
	return res;
}
