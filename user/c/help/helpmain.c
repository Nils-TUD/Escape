/**
 * $Id: help.c 307 2009-09-06 15:29:49Z nasmussen $
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
#include <esc/cmdargs.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <app.h>

#define MAX_APPS		256

static s32 appCompare(const void *a,const void *b);
static bool addApp(const char *def,char *search,bool listUser,bool system);
static char *readApp(char *path);

static void usage(void) {
	fprintf(stderr,"Usage: help [<app>] | [--detail][--user][--system]\n");
	exit(EXIT_FAILURE);
}

/* the buildin commands */
static const char *cdCmd = "name: \"cd\""
		"start: \"cd <dir>\""
		"type: \"user\""
		"desc: \"Change to given directory\"";
static const char *envCmd = "name: \"env\""
		"start: \"env [<name>|<name>=<value>]\""
		"type: \"user\""
		"desc: \"Print or set env-variable(s)\"";
static const char *pwdCmd = "name: \"pwd\""
		"start: \"pwd\""
		"type: \"user\""
		"desc: \"Print the working directory\"";
static const char *echoCmd = "name: \"echo\""
		"start: \"echo <string> ...\""
		"type: \"user\""
		"desc: \"Prints the given arguments\"";

static u32 appCount = 0;
static sApp *apps[MAX_APPS];

int main(int argc,char **argv) {
	sDirEntry e;
	tFD dirFd;
	u32 i;
	char *appdef;
	char *pathEnd;
	char path[MAX_PATH_LEN] = "/apps/";
	char *app = NULL;
	bool listUser = false;
	bool listSys = false;
	bool detail = false;

	if(isHelpCmd(argc,argv))
		usage();

	for(i = 1; i < argc; i++) {
		if(strncmp(argv[i],"--",2) == 0) {
			if(strcmp(argv[i] + 2,"detail") == 0)
				detail = true;
			else if(strcmp(argv[i] + 2,"system") == 0)
				listSys = true;
			else if(strcmp(argv[i] + 2,"user") == 0)
				listUser = true;
			else
				usage();
		}
		else {
			if(app)
				usage();
			app = argv[i];
		}
	}

	if(app || listUser) {
		addApp(cdCmd,app,listUser,listSys);
		addApp(pwdCmd,app,listUser,listSys);
		addApp(envCmd,app,listUser,listSys);
		addApp(echoCmd,app,listUser,listSys);
	}

	if(app || listUser || listSys) {
		dirFd = opendir("/apps");
		if(dirFd < 0)
			error("Unable to open /apps");

		pathEnd = path + strlen(path);
		while(readdir(&e,dirFd)) {
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;
			strcpy(pathEnd,e.name);
			appdef = readApp(path);
			if(!appdef) {
				closedir(dirFd);
				error("Unable to read '%s'",path);
			}

			if(!addApp(appdef,app,listUser,listSys)) {
				free(appdef);
				closedir(dirFd);
				error("Unable to parse app in '%s'",path);
			}
			free(appdef);
		}
		closedir(dirFd);
	}

	if(app) {
		for(i = 0; i < appCount; i++) {
			if(strcmp(apps[i]->name,app) == 0) {
				printf("Synopsis:		%s\n",apps[i]->start);
				printf("Description:	%s\n",apps[i]->desc);
				break;
			}
		}
	}
	else if(listUser || listSys) {
		qsort(apps,appCount,sizeof(sApp*),appCompare);

		for(i = 0; i < appCount; i++) {
			if(detail) {
				printf("%s:\n",apps[i]->name);
				printf("	Synopsis:		%s\n",apps[i]->start);
				printf("	Description:	%s\n",apps[i]->desc);
				printf("\n");
			}
			else
				printf("%s ",apps[i]->name);
		}
		if(!detail)
			printf("\n");
	}
	else {
		/*printf("Currently you can use the following commands/programs:\n");
		printf("	cat [<file>]					Read file or from STDIN and print\n");
		printf("	colortbl						Print a table with all fg- and bg-colors\n");
		printf("	date [<format>]					Print date (with specified format or %%c)\n");
		printf("	kill <pid>						Kill process\n");
		printf("	libctest						Run libc-tests\n");
		printf("	test <testName> ...				Run test-module <testName>\n");
		printf("	login							Just for fun ;)\n");
		printf("	ls [-ali] [<dir>]				List current or specified directory\n");
		printf("	progress						Shows a progress-bar\n");
		printf("	ps [-t]							Print processes\n");
		printf("	echo <string> ...				Print given arguments\n");
		printf("	env [<name>|<name>=<value>]		Print or set env-variable(s)\n");
		printf("	help							Print this message\n");
		printf("	cd <dir>						Change to given directory\n");
		printf("	pwd								Print current directory\n");
		printf("	wc								Count and/or print words\n");
		printf("	stat <file>						Print file-information\n");
		printf("	less [<file>]					Allows navigation through <file> or STDIN\n");
		printf("	dump [-n <b>][-f <f>][<file>]	Dumps bytes from <file> or STDIN to STDOUT\n");
		printf("	grep <pattern> [<file>]			Prints all lines that match <pattern>\n");
		printf("	ln <target> <name>				Lets <name> point to <target>\n");
		printf("	mkdir <dir> ...					Creates the given directories\n");
		printf("	rm <file> ...					Removes the given files\n");
		printf("	rmdir <dir> ...					Removes the given (empty!) directories\n");
		printf("	sync							Flushes the filesystem-buffers\n");
		printf("	write <file>					Reads from STDIN and writes it to <file>\n");
		printf("	mount <device> <path> <type>	Mounts <device> @ <path> with fs <type>\n");
		printf("	umount <path>					Unmounts the device @ <path>\n");
		printf("\n");
		printf("	gui								Start GUI\n");
		printf("									(be carefull: this is a oneway-ticket ;))\n");
		printf("	guishell						A shell for the GUI\n");*/

		printf("Some words to 'help':\n");
		printf("	Use 'help --user' to get a list of all user-applications\n");
		printf("	Use 'help --system' to get a list of all system-applications\n");
		printf("	Use 'help ... --detail' to see usage-details\n");
		printf("	Use 'help <cmd>' to get help for a specific applicaiton\n");

		printf("\n");
		printf("Execute a command with '--help', '-h' or '-?' to get further information\n");

		printf("\n");
		printf("Additionally there is a basic shell-'language', that supports '|', '\"' and ';'\n");
		printf("So for example you can do:\n");
		printf("	ls; ps; echo \"test\";\n");
		printf("	ls | cat; ps\n");
		printf("	echo \"word1\" word2 \"word3 and 4\" | wc\n");
		printf("	cat /bigfile | less\n");
		printf("	ps | wc | wc -p\n");

		printf("\n");
		printf("Other:\n");
		printf("	Tab-Completion works for programs in /bin and files/directories\n");
		printf("	You can send EOF by CTRL+D and kill the current process with CTRL+C\n");
		printf("	You can scroll the screen with shift + up/down/pageUp/-Down\n");
		printf("	You can switch between different vterms via CTRL+1/2/...\n");
		printf("\n");
	}
	return EXIT_SUCCESS;
}

static s32 appCompare(const void *a,const void *b) {
	sApp *aa = *(sApp**)a;
	sApp *ab = *(sApp**)b;
	return strcmp(aa->name,ab->name);
}

static bool addApp(const char *def,char *search,bool listUser,bool listSys) {
	sApp *app = (sApp*)malloc(sizeof(sApp));
	if(app_fromString(def,app) == NULL)
		return false;
	if((search && strcmp(app->name,search) == 0) ||
		(app->type == APP_TYPE_USER && listUser) ||
		(app->type != APP_TYPE_USER && listSys)) {
		apps[appCount++] = app;
	}
	else
		free(app);
	return true;
}

static char *readApp(char *path) {
	tFD fileFd;
	const u32 incSize = 128;
	u32 count;
	u32 appSize = incSize;
	u32 appPos = 0;
	char *app = (char*)malloc(appSize);
	if(!app) {
		printe("Unable to alloc mem for app");
		return NULL;
	}
	fileFd = open(path,IO_READ);
	if(fileFd < 0) {
		printe("Unable to open '%s'",path);
		return NULL;
	}

	while((count = read(fileFd,app + appPos,incSize)) > 0) {
		appPos += count;
		appSize += incSize;
		app = (char*)realloc(app,appSize);
		if(!app) {
			printe("Unable to alloc mem for app");
			return NULL;
		}
	}
	app[appPos] = '\0';

	close(fileFd);
	return app;
}
