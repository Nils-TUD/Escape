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
#include "driver.h"

static bool loadDriver(sDriverLoad **loads,sDriverLoad *load);
static sDriverLoad *getDriver(sDriverLoad **loads,char *name);

/* the already loaded drivers; we don't want to load a driver twice */
static sSLList *loadedDrivers;

bool loadDrivers(sDriverLoad **loads) {
	sDriverLoad *load;
	loadedDrivers = sll_create();
	if(loadedDrivers == NULL)
		return false;

	load = *loads;
	while(load != NULL) {
		if(!loadDriver(loads,load))
			return false;
		load = *++loads;
	}
	return true;
}

void printDrivers(sDriverLoad **loads) {
	u8 i;
	printf("Loads:\n");
	if(loads != NULL) {
		sDriverLoad *load = *loads;
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

static bool loadDriver(sDriverLoad **loads,sDriverLoad *load) {
	u32 i,j;
	s32 res,child;
	char path[MAX_DRIVER_PATH_LEN + 1] = "/sbin/";
	char drvName[MAX_DRIVER_PATH_LEN + 1] = "";
	char *sname;
	sFileInfo info;
	sSLNode *n;

	/* at first check if we've already loaded the driver */
	for(n = sll_begin(loadedDrivers); n != NULL; n = n->next) {
		if(strcmp((char*)n->data,load->name) == 0)
			return true;
	}

	/* load dependencies */
	for(i = 0; i < load->depCount; i++) {
		sDriverLoad *l = getDriver(loads,load->deps[i]);
		if(l != NULL) {
			if(!loadDriver(loads,l))
				return false;
		}
	}

	/* now load the driver */
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
	sname = drvName;
	for(i = 0; i < load->waitCount; i++) {
		strcpy(sname,load->waits[i]);
		j = 0;
		do {
			res = stat(drvName,&info);
			if(res < 0)
				sleep(10);
		}
		while(j++ < MAX_WAIT_RETRIES && res < 0);
		if(res < 0) {
			printe("The driver '%s' was not found after %d retries",drvName,MAX_WAIT_RETRIES);
			return false;
		}
	}

	/* insert in loaded-list so that we don't load a driver twice */
	sll_append(loadedDrivers,load->name);

	return true;
}

static sDriverLoad *getDriver(sDriverLoad **loads,char *name) {
	sDriverLoad *load = *loads;
	while(load != NULL) {
		if(strcmp(load->name,name) == 0)
			return load;
		load = *++loads;
	}
	return NULL;
}
