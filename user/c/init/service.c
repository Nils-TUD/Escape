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
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esc/io.h>
#include <fsinterface.h>
#include <string.h>
#include <sllist.h>
#include <stdlib.h>
#include "service.h"

static bool loadService(sServiceLoad **loads,sServiceLoad *load);
static sServiceLoad *getService(sServiceLoad **loads,char *name);

/* the already loaded services; we don't want to load a service twice */
static sSLList *loadedServices;

bool loadServices(sServiceLoad **loads) {
	sServiceLoad *load;
	loadedServices = sll_create();
	if(loadedServices == NULL)
		return false;

	load = *loads;
	while(load != NULL) {
		if(!loadService(loads,load))
			return false;
		load = *++loads;
	}
	return true;
}

void printServices(sServiceLoad **loads) {
	u8 i;
	printf("Loads:\n");
	if(loads != NULL) {
		sServiceLoad *load = *loads;
		while(load != NULL) {
			printf("\tname: '%s' waits(%d): ",load->name,load->waitCount);
			for(i = 0; i < load->waitCount; i++) {
				printf("'%s'",load->waits[i]);
				if(i < load->waitCount - 1)
					printf(",");
			}
			printf(" deps(%d): ",load->depCount);
			for(i = 0; i < load->depCount; i++) {
				printf("'%s'",load->deps[i]);
				if(i < load->depCount - 1)
					printf(",");
			}
			printf("\n");
			load = *++loads;
		}
	}
}

static bool loadService(sServiceLoad **loads,sServiceLoad *load) {
	u32 i,j;
	s32 res,child;
	char path[MAX_SERVICE_PATH_LEN + 1] = "/sbin/";
	char servName[MAX_SERVICE_PATH_LEN + 1] = "";
	char *sname;
	sFileInfo info;
	sSLNode *n;

	/* at first check if we've already loaded the service */
	for(n = sll_begin(loadedServices); n != NULL; n = n->next) {
		if(strcmp((char*)n->data,load->name) == 0)
			return true;
	}

	/* load dependencies */
	for(i = 0; i < load->depCount; i++) {
		sServiceLoad *l = getService(loads,load->deps[i]);
		if(l != NULL) {
			if(!loadService(loads,l))
				return false;
		}
	}

	/* now load the service */
	strcat(path,load->name);
	child = fork();
	if(child == 0) {
		exec(path,NULL);
		printe("Exec of '%s' failed",path);
		exit(EXIT_FAILURE);
	}
	else if(child < 0) {
		printe("Fork of '%s' failed",path);
		return false;
	}

	/* wait for all specified waits */
	sname = servName;
	for(i = 0; i < load->waitCount; i++) {
		strcpy(sname,load->waits[i]);
		j = 0;
		do {
			res = stat(servName,&info);
			if(res < 0)
				sleep(10);
		}
		while(j++ < MAX_WAIT_RETRIES && res < 0);
		if(res < 0) {
			printe("The service '%s' was not found after %d retries",servName,MAX_WAIT_RETRIES);
			return false;
		}
	}

	/* insert in loaded-list so that we don't load a service twice */
	sll_append(loadedServices,load->name);

	return true;
}

static sServiceLoad *getService(sServiceLoad **loads,char *name) {
	sServiceLoad *load = *loads;
	while(load != NULL) {
		if(strcmp(load->name,name) == 0)
			return load;
		load = *++loads;
	}
	return NULL;
}
