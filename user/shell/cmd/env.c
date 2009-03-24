/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <bufio.h>
#include <env.h>
#include <messages.h>
#include <heap.h>
#include <string.h>
#include "env.h"

s32 shell_cmdEnv(u32 argc,char **argv) {
	if(argc < 2) {
		u32 i,len;
		char *backup;
		char *val,*name;
		for(i = 0; (name = getEnvByIndex(i)) != NULL; i++) {
			/* save name (getEnv will overwrite it) */
			len = strlen(name);
			backup = (char*)malloc(len + 1);
			memcpy(backup,name,len + 1);

			val = getEnv(name);
			if(val != NULL)
				printf("%s=%s\n",backup,val);
		}
	}
	else if(argc == 2) {
		char *val;
		u32 pos = strchri(argv[1],'=');
		if(argv[1][pos] == '\0') {
			val = getEnv(argv[1]);
			if(val != NULL)
				printf("%s=%s\n",argv[1],val);
		}
		else {
			argv[1][pos] = '\0';
			setEnv(argv[1],argv[1] + pos + 1);
			val = getEnv(argv[1]);
			if(val != NULL)
				printf("%s=%s\n",argv[1],val);
		}
	}
	else {
		printf("Usage: %s [<name>|<name>=<value>]\n");
		return 1;
	}

	return 0;
}
