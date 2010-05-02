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
#include <io/streams.h>
#include <util/vector.h>
#include <string.h>
#include <sllist.h>
#include "idriver.h"

static bool loadDriver(sVector *loads,sDriverLoad *load);
static sDriverLoad *getDriver(sVector *loads,char *name);

/* the already loaded drivers; we don't want to load a driver twice */
static sSLList *loadedDrivers;

bool loadDrivers(sVector *loads) {
	sDriverLoad *load;
	loadedDrivers = sll_create();
	if(loadedDrivers == NULL)
		return false;

	vforeach(loads,load) {
		if(!loadDriver(loads,load))
			return false;
	}
	return true;
}

void printDrivers(sVector *loads) {
	sDriverLoad *load;
	cout->writes(cout,"Loads:\n");
	if(loads != NULL) {
		vforeach(loads,load) {
			char *wname,*dname;
			cout->format(cout,"\tname: '%s' waits(%d): ",load->name,load->waits->count);
			vforeach(load->waits,wname)
				cout->format(cout,"'%s' ",wname);
			cout->format(cout," deps(%d): ",load->deps->count);
			vforeach(load->deps,dname)
				cout->format(cout,"'%s' ",dname);
			cout->format(cout,"\n");
		}
	}
}

static bool loadDriver(sVector *loads,sDriverLoad *load) {
	u32 j;
	s32 res,child;
	char path[MAX_DRIVER_PATH_LEN + 1] = "/sbin/";
	char drvName[MAX_DRIVER_PATH_LEN + 1] = "";
	char *sname,*dname,*wname;
	sFileInfo info;
	sSLNode *n;

	/* at first check if we've already loaded the driver */
	for(n = sll_begin(loadedDrivers); n != NULL; n = n->next) {
		if(strcmp((char*)n->data,load->name) == 0)
			return true;
	}

	/* load dependencies */
	vforeach(load->deps,dname) {
		sDriverLoad *l = getDriver(loads,dname);
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
		cerr->format(cerr,"Exec of '%s' failed\n",path);
		exit(EXIT_FAILURE);
	}
	else if(child < 0) {
		cerr->format(cerr,"Fork of '%s' failed\n",path);
		return false;
	}

	/* wait for all specified waits */
	sname = drvName;
	vforeach(load->waits,wname) {
		strcpy(sname,wname);
		j = 0;
		do {
			res = stat(drvName,&info);
			if(res < 0)
				sleep(10);
		}
		while(j++ < MAX_WAIT_RETRIES && res < 0);
		if(res < 0) {
			cerr->format(cerr,"The driver '%s' was not found after %d retries\n",
					drvName,MAX_WAIT_RETRIES);
			return false;
		}
	}

	/* insert in loaded-list so that we don't load a driver twice */
	sll_append(loadedDrivers,load->name);

	return true;
}

static sDriverLoad *getDriver(sVector *loads,char *name) {
	sDriverLoad *load;
	vforeach(loads,load) {
		if(strcmp(load->name,name) == 0)
			return load;
	}
	return NULL;
}
