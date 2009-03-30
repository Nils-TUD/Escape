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

#define MAX_SNAME_LEN			50
#define MAX_SERVICE_PATH_LEN	255

typedef struct {
	/* the service-name */
	char *name;
	/* an array of service-names to wait for */
	u8 waitCount;
	char **waits;
	/* an array of dependencies to load before */
	u8 depCount;
	char **deps;
} sServiceLoad;

/**
 * Parses the given services-file and creates an array of sServiceLoad's.
 *
 * @param services the service-file-content
 * @return an array of sServiceLoad's
 */
static sServiceLoad **parseServices(char *services);

/**
 * Parses one service from the given line (until newline or \0) and fills the
 * given service-load
 *
 * @param line the line-beginning
 * @param serv the service-load to fill
 * @return the position where to continue
 */
static char *parseService(char *line,sServiceLoad *serv);

/**
 * Loads the given services
 *
 * @param loads the services to load
 * @return true if successfull
 */
static bool loadServices(sServiceLoad **loads);

/**
 * Loads the given service
 *
 * @param loads all services
 * @param load the service to load
 * @return true if successfull
 */
static bool loadService(sServiceLoad **loads,sServiceLoad *load);

/**
 * Searches for the service-load with given name
 *
 * @param loads all services
 * @param name the service-name
 * @return the service-load or NULL
 */
static sServiceLoad *getService(sServiceLoad **loads,char *name);

/**
 * Prints the given service-loads
 *
 * @param loads the service-loads
 */
static void printServices(sServiceLoad **loads);

/**
 * Loads the service-dependency-file
 * @return the file-content
 */
static char *getServices(void);

/* the already loaded services; we don't want to load a service twice */
static sSLList *loadedServices;
static sServiceLoad **services = NULL;

int main(void) {
	tFD fd;
	char *servDefs;

	/* wait for fs; we need it for exec */
	do {
		fd = open("services:/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
	}
	while(fd < 0);
	close(fd);

	/* now read the services we should load */
	servDefs = getServices();
	if(servDefs == NULL) {
		printLastError();
		return 1;
	}

	/* parse them */
	services = parseServices(servDefs);
	if(services == NULL) {
		debugf("Unable to parse service-file\n");
		return 1;
	}

	/* finally load them */
	if(!loadServices(services)) {
		debugf("Unable to load services\n");
		return 1;
	}

	/* now load the shell */
	if(fork() == 0) {
		const char *args[] = {"file:/bin/shell","vterm0",NULL};
		exec((char*)"file:/bin/shell",args);
		exit(0);
	}
	/* TODO temporary ;) */
	if(fork() == 0) {
		const char *args[] = {"file:/bin/shell","vterm1",NULL};
		exec((char*)"file:/bin/shell",args);
		exit(0);
	}

	/* loop and wait forever */
	while(1)
		wait(EV_NOEVENT);
	return 0;
}

static sServiceLoad **parseServices(char *servFile) {
	u32 servSize = 8;
	u32 servPos = 0;
	sServiceLoad **servBak;
	sServiceLoad **servs;
	sServiceLoad **serv;
	sServiceLoad *s;
	char *str;

	/* create array */
	servs = (sServiceLoad**)malloc(servSize * sizeof(sServiceLoad*));
	if(servs == NULL)
		return NULL;

	servBak = servs;
	str = servFile;
	while(*str) {
		/* ";" at the line-start is a comment */
		if(*str != ';') {
			/* increase array-size? leave one slot for NULL */
			if(servPos >= servSize - 1) {
				servSize += 8;
				servs = (sServiceLoad**)realloc(servs,servSize * sizeof(sServiceLoad*));
				if(servs == NULL)
					goto failed;
			}

			/* alloc mem for service-load */
			s = (sServiceLoad*)malloc(sizeof(sServiceLoad));
			if(s == NULL)
				goto failed;

			/* parse this line */
			str = parseService(str,s);
			if(str == NULL)
				goto failed;
			/* if the line was empty (name == NULL), simply ignore it */
			if(s->name != NULL)
				servs[servPos++] = s;
			else
				free(s);
		}

		/* wait until newline */
		while(*str && *str != '\n')
			str++;
		if(!*str)
			break;
		str++;
	}

	/* terminate */
	servs[servPos] = NULL;
	return servs;

failed:
	/* we have to free the already allocated mem */
	serv = servBak + servPos - 1;
	while(serv > servBak) {
		free((*serv)->name);
		free((*serv)->deps);
		free((*serv)->waits);
		free(*serv);
		serv--;
	}
	free(servBak);
	return NULL;
}

static char *parseService(char *line,sServiceLoad *serv) {
	char *s,*brk,*lineStart;
	u32 i,len,size;
	u8 *count;
	char ***array;

	/* search for name-end */
	s = strpbrk(line,":\n");
	if(s == NULL)
		s = line + strlen(line);

	/* empty name? */
	if(s - line == 0)
		return s;

	/* set name */
	serv->name = (char*)malloc(s - line + 1);
	if(serv->name == NULL)
		return false;
	strncpy(serv->name,line,s - line);
	serv->name[s - line] = '\0';

	/* create wait-array */
	serv->waitCount = 0;
	serv->waits = (char**)malloc(8 * sizeof(char*));
	if(serv->waits == NULL) {
		free(serv->name);
		return NULL;
	}

	/* create dep-array */
	serv->depCount = 0;
	serv->deps = (char**)malloc(8 * sizeof(char*));
	if(serv->deps == NULL) {
		free(serv->name);
		free(serv->waits);
		return NULL;
	}

	/* no deps and waits? */
	if(*s == '\0' || *s == '\n')
		return s;

	/* first load waits, then load deps (syntax is the same) */
	line = s + 1;
	lineStart = line;
	for(i = 0; i < 2; i++) {
		size = 8;
		if(i == 0) {
			array = &serv->waits;
			count = &serv->waitCount;
		}
		else {
			array = &serv->deps;
			count = &serv->depCount;
		}

		while(1) {
			/* skip whitespace */
			while(*line && *line != '\n' && (*line == ' ' || *line == '\t'))
				line++;

			/* search for sep */
			if(!*line || *line == '\n')
				break;
			brk = strpbrk(line,",:\t \n");
			/* if we're at the end, use all until the end */
			if(brk == NULL)
				brk = lineStart + strlen(lineStart);

			len = brk - line;
			if(len > 0) {
				/* increase array-size? */
				if(*count >= size) {
					size += 8;
					*array = (char**)realloc(*array,size * sizeof(char*));
					if(*array == NULL)
						goto failed;
				}

				/* copy wait-name */
				(*array)[*count] = (char*)malloc(len + 1);
				if((*array)[*count] == NULL)
					goto failed;
				strncpy((*array)[*count],line,len);
				(*array)[*count][len] = '\0';
				(*count)++;
			}

			/* continue behind the separator */
			line = brk + 1;
			/* if we are finished with this line or type (wait,dep), break */
			if(!*brk || *brk == '\n' || *brk == ':') {
				/* line finished? so we one step too far */
				if(*brk && *brk == '\n')
					line--;
				break;
			}
		}
	}

	return line;

failed:
	free(serv->name);
	free(serv->deps);
	free(serv->waits);
	return NULL;
}

static bool loadServices(sServiceLoad **loads) {
	loadedServices = sll_create();
	if(loadedServices == NULL)
		return false;

	sServiceLoad *load = *loads;
	while(load != NULL) {
		if(!loadService(loads,load))
			return false;
		load = *++loads;
	}
	return true;
}

static bool loadService(sServiceLoad **loads,sServiceLoad *load) {
	u32 i;
	tFD fd;
	char path[MAX_SERVICE_PATH_LEN + 1] = "file:/services/";
	char servName[MAX_SERVICE_PATH_LEN + 1] = "services:/";
	char *sname;
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
	if(fork() == 0) {
		exec(path,NULL);
		/* just to be sure... */
		exit(0);
	}

	/* wait for all specified waits */
	sname = servName + 10;
	for(i = 0; i < load->waitCount; i++) {
		strcpy(sname,load->waits[i]);
		do {
			fd = open(servName,IO_READ);
			if(fd < 0)
				yield();
		}
		while(fd < 0);
		close(fd);
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

static void printServices(sServiceLoad **loads) {
	u8 i;
	debugf("Loads:\n");
	if(loads != NULL) {
		sServiceLoad *load = *loads;
		while(load != NULL) {
			debugf("\tname: '%s' waits(%d): ",load->name,load->waitCount);
			for(i = 0; i < load->waitCount; i++) {
				debugf("'%s'",load->waits[i]);
				if(i < load->waitCount - 1)
					debugf(",");
			}
			debugf(" deps(%d): ",load->depCount);
			for(i = 0; i < load->depCount; i++) {
				debugf("'%s'",load->deps[i]);
				if(i < load->depCount - 1)
					debugf(",");
			}
			debugf("\n");
			load = *++loads;
		}
	}
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
