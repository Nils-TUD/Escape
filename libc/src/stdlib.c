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

/* macro to get a pointer to element <i> from <array> with a size of <elSize> per element */
#define QSORT_AGET(array,elSize,i) ((u32)(array) + (elSize) * (i))

/**
 * Qsort-implementation
 */
static void _qsort(void *base,size_t num,size_t size,fCompare cmp,int left,int right);

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
		const char *args[] = {"/bin/shell","-e",cmd,NULL};
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

void qsort(void *base,size_t num,size_t size,fCompare cmp) {
	_qsort(base,num,size,cmp,0,num - 1);
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

static void _qsort(void *base,size_t num,size_t size,fCompare cmp,int left,int right) {
	void *piv;
	unsigned char *iptr,*jptr;
	unsigned long int ptr;
	int i,j;
	size_t x;

	/* Nur noch max. 1 Element? */
	if(left >= right)
		return;

	piv = (void*)QSORT_AGET(base,size,(left + right) / 2);
	i = left;
	j = right;
	while(i <= j) {
		/* right until the element is >= piv */
		ptr = QSORT_AGET(base,size,i);
		while(cmp((void*)ptr,piv) < 0) {
			ptr += size;
			i++;
		}

		/* left until the element is <= piv */
		ptr = QSORT_AGET(base,size,j);
		while(cmp((void*)ptr,piv) > 0) {
			ptr -= size;
			j--;
		}

		/* swap if i <= j */
		if(i <= j) {
			iptr = (unsigned char*)QSORT_AGET(base,size,i);
			jptr = (unsigned char*)QSORT_AGET(base,size,j);
			/* *iptr XOR *jptr == 0 for *iptr == *jptr ... */
			if(memcmp(iptr,jptr,size) != 0) {
				for(x = 0; x < size; x++) {
					 *iptr ^= *jptr;
					 *jptr ^= *iptr;
					 *iptr ^= *jptr;
					 iptr++;
					 jptr++;
				}
			}
			i++;
			j--;
		}
	}

	_qsort(base,num,size,cmp,left,j);
	_qsort(base,num,size,cmp,i,right);
}
