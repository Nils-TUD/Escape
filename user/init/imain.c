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

/**
 * Loads the service-dependency-file
 * @return the file-content
 */
static char *getServices(void);

/**
 * Loads all services with given dependency-file
 *
 * @param services the dependencies
 * @return true if successfull
 */
static bool loadServices(char *services);

/**
 * Loads the service with given name and takes care of dependencies
 *
 * @param services the dependencies
 * @param name the service to load
 * @return true if successfull
 */
static bool loadService(char *services,char *name);

/* the already loaded services; we don't want to load a service twice */
static sSLList *loadedServices;

s32 main(void) {
	tFD fd;
	char *services;

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

	/* open stdin */
	if(open("services:/vterm",IO_READ) < 0) {
		debugf("Unable to open 'services:/vterm' for STDIN\n");
		return 1;
	}

	/* open stdout */

	if((fd = open("services:/vterm",IO_WRITE)) < 0) {
		debugf("Unable to open 'services:/vterm' for STDOUT\n");
		return 1;
	}

	/* dup stdout to stderr */
	if(dupFd(fd) < 0) {
		debugf("Unable to duplicate STDOUT to STDERR\n");
		return 1;
	}

	/* now load the shell */
	if(fork() == 0) {
		exec((char*)"file:/apps/shell",NULL);
		exit(0);
	}

	/* loop forever, and don't waste too much cpu-time */
	/* TODO we should improve this some day ;) */
	while(1) {
		yield();
	}
	return 0;
}

static bool loadServices(char *services) {
	char *str;
	u32 pos;
	char *tmpName;

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
		tmpName = (char*)malloc((str - (services + pos)) * sizeof(char));
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

static bool loadService(char *services,char *name) {
	char *str;
	tFD fd;
	char servPath[MAX_SERVICE_PATH_LEN] = "services:/";
	char path[MAX_SERVICE_PATH_LEN] = "file:/services/";
	u32 p,pos,nameLen;
	sSLNode *n;

	/* at first check if we've already loaded the service */
	for(n = sll_begin(loadedServices); n != NULL; n = n->next) {
		if(strcmp((char*)n->data,name) == 0)
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
		char *servName = services + pos;
		char *tmpName;
		char *start;

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
			tmpName = (char*)malloc((str - start) * sizeof(char));
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
		start = malloc((p + 1) * sizeof(char));
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

static char *getServices(void) {
	const u32 stepSize = 128 * sizeof(u8);
	tFD fd;
	u32 c,pos = 0,bufSize = stepSize;
	char *buffer;

	/* open file */
	fd = open("file:/etc/services",IO_READ);
	if(fd < 0)
		return NULL;

	/* create buffer */
	buffer = (char*)malloc(stepSize);
	if(buffer == NULL)
		return NULL;

	/* read file */
	while((c = read(fd,buffer + pos,stepSize - 1)) > 0) {
		bufSize += stepSize;
		buffer = (char*)realloc(buffer,bufSize);
		if(buffer == NULL)
			return NULL;

		pos += c;
	}

	/* terminate */
	*(buffer + pos) = '\0';

	close(fd);
	return buffer;
}
