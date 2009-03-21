/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <proc.h>
#include <string.h>
#include <heap.h>
#include <sllist.h>
#include <debug.h>

#define MAX_SERVICE_PATH_LEN	255

static s8 *getServices(void);
static bool loadServices(s8 *services);
static bool loadService(s8 *services,s8 *name);

static sSLList *loadedServices;

s32 main(void) {
	s32 fd;
	s8 *services;

	/* wait for fs; we need it for exec */
	do {
		fd = open("services:/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
	}
	while(fd < 0);
	close(fd);

	/* now read the services we should load */
	services = getServices();
	if(services == NULL) {
		printLastError();
		return 1;
	}

	/* load them */
	if(!loadServices(services)) {
		printLastError();
		return 1;
	}

	/* create stdin, stdout and stderr */
	if(!initIO()) {
		debugf("Unable to init IO\n");
		return 1;
	}

	/* now load the shell */
	if(fork() == 0) {
		exec("file:/apps/shell",NULL);
		exit(0);
	}

	/* loop forever, and don't waste too much cpu-time */
	/* TODO we should improve this some day ;) */
	while(1) {
		yield();
	}
	return 0;
}

static bool loadServices(s8 *services) {
	s8 *str;
	u32 pos;
	s8 *tmpName;

	loadedServices = sll_create();
	if(loadedServices == NULL)
		return false;

	pos = 0;
	while(*(services + pos)) {
		/* walk to the name-end */
		str = services + pos;
		while(*str && *str != ':')
			str++;
		if(!*str)
			break;

		/* we don't want to manipulate <services>... */
		tmpName = (s8*)malloc((str - (services + pos)) * sizeof(s8));
		if(tmpName == NULL)
			return false;
		memcpy(tmpName,services + pos,str - (services + pos));
		*(tmpName + (str - (services + pos))) = '\0';
		/* load the service (dependencies first) */
		if(!loadService(services,tmpName)) {
			free(tmpName);
			return false;
		}
		free(tmpName);

		/* walk to next line */
		while(*str && *str != '\n')
			str++;
		if(!*str)
			break;
		pos += str - (services + pos) + 1;
	}
	return true;
}

static bool loadService(s8 *services,s8 *name) {
	s8 *str;
	s32 fd;
	s8 servPath[MAX_SERVICE_PATH_LEN] = "services:/";
	s8 path[MAX_SERVICE_PATH_LEN] = "file:/services/";
	u32 p,pos,nameLen;
	sSLNode *n;

	/* at first check if we've already loaded the service */
	for(n = sll_begin(loadedServices); n != NULL; n = n->next) {
		if(strcmp((s8*)n->data,name) == 0)
			return true;
	}

	pos = 0;
	nameLen = strlen(name);
	while(*(services + pos)) {
		/* walk to the name-end */
		str = services + pos;
		while(*str && *str != ':')
			str++;
		if(!*str)
			break;

		/* found the service? */
		p = str - (services + pos);
		if(p == nameLen && strncmp(services + pos,name,p) == 0)
			break;

		/* walk to next line */
		while(*str && *str != '\n')
			str++;
		if(!*str)
			break;
		pos += (str - (services + pos)) + 1;
	}

	/* have we found it? */
	if(*str) {
		s8 *servName = services + pos;
		s8 *tmpName;
		s8 *start;

		/* skip ':' */
		str++;
		/* load dependencies */
		while(1) {
			/* skip whitespace */
			while(*str && (*str == ' ' || *str == '\t'))
				str++;
			/* no dependencies? */
			if(!*str || *str == '\n')
				break;

			/* read dependency */
			start = str;
			while(*str && *str != ',' && *str != '\n')
				str++;
			if(!*str)
				break;

			/* load dependency */
			tmpName = (s8*)malloc((str - start) * sizeof(s8));
			if(tmpName == NULL)
				return false;
			memcpy(tmpName,start,str - start);
			*(tmpName + (str - start)) = '\0';
			if(!loadService(services,tmpName)) {
				free(tmpName);
				return false;
			}
			free(tmpName);

			if(*str == '\n')
				break;
			/* skip ',' */
			str++;
		}

		/* ensure that the name is not too long */
		if(strlen(path) + p >= MAX_SERVICE_PATH_LEN || strlen(servPath) + p >= MAX_SERVICE_PATH_LEN)
			return false;

		/* dependencies are running, so fork & exec */
		if(fork() == 0) {
			strncat(path,servName,p);
			exec(path,NULL);
			/* just to be sure */
			exit(0);
		}

		/* wait until the service is available */
		strncat(servPath,servName,p);
		do {
			fd = open(servPath,IO_READ);
			if(fd < 0)
				yield();
		}
		while(fd < 0);
		close(fd);

		/* insert in loaded-list, so that we don't load a service twice */
		start = malloc((p + 1) * sizeof(s8));
		if(start == NULL)
			return false;
		memcpy(start,servName,p);
		*(start + p) = '\0';
		sll_append(loadedServices,start);
	}
	else {
		debugf("Unable to find '%s'\n",name);
		return false;
	}

	return true;
}

static s8 *getServices(void) {
	const u32 stepSize = 128 * sizeof(u8);
	s32 fd;
	u32 c,pos = 0,bufSize = stepSize;
	s8 *buffer;

	/* open file */
	fd = open("file:/etc/services",IO_READ);
	if(fd < 0)
		return NULL;

	/* create buffer */
	buffer = (s8*)malloc(stepSize);
	if(buffer == NULL)
		return NULL;

	/* read file */
	while((c = read(fd,buffer + pos,stepSize - 1)) > 0) {
		bufSize += stepSize;
		buffer = (s8*)realloc(buffer,bufSize);
		if(buffer == NULL)
			return NULL;

		pos += c;
	}

	/* terminate */
	*(buffer + pos) = '\0';

	close(fd);
	return buffer;
}
