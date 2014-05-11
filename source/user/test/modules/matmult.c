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
#include <esc/time.h>
#include <esc/proc.h>
#include <stdlib.h>
#include <stdio.h>

#include "../modules.h"

#define SIZE			2048

static void fill(int *mat) {
	srand(rdtsc());
	for(size_t i = 0; i < SIZE; ++i) {
		for(size_t j = 0; j < SIZE; ++j)
			mat[i * SIZE + j] = rand();
	}
}

static void matmult(int *m1,int *m2,int *res) {
	for(size_t i = 0; i < SIZE; ++i) {
		for(size_t j = 0; j < SIZE; ++j) {
			int v = 0;
			for(size_t k = 0; k < SIZE; ++k)
				v += m1[i * SIZE + j] * m2[j * SIZE + i];
			res[i * SIZE + j] = v;
		}
	}
}

int mod_matmult(int argc,char *argv[]) {
	int parallel = sysconf(CONF_CPU_COUNT);
	int iterations = 1;
	if(argc > 2)
		parallel = atoi(argv[2]);
	if(argc > 3)
		iterations = atoi(argv[3]);

	for(int i = 0; i < parallel; ++i) {
		int pid = fork();
		if(pid == 0) {
			int *m1 = (int*)malloc(SIZE * SIZE * sizeof(int));
			int *m2 = (int*)malloc(SIZE * SIZE * sizeof(int));
			int *res = (int*)malloc(SIZE * SIZE * sizeof(int));
			if(!m1 || !m2 || !res)
				error("Unable to allocated %zu bytes",SIZE * SIZE * sizeof(int));
			fill(m1);
			fill(m2);
			uint64_t start = rdtsc();
			for(int x = 0; x < iterations; ++x)
				matmult(m1,m2,res);
			uint64_t end = rdtsc();
			printf("time of %d: %Lu\n",getpid(),end - start);
			return EXIT_SUCCESS;
		}
	}
	for(int i = 0; i < parallel; ++i)
		waitchild(NULL);
	return EXIT_SUCCESS;
}
