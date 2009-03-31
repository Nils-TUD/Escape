/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include "help.h"

s32 shell_cmdHelp(u32 argc,char **argv) {
	UNUSED(argc);
	UNUSED(argv);

	printf("Currently you can use the following commands/programs:\n");
	printf("\tcat [<file>]					Read file or from STDIN and print\n");
	printf("\tdate [<format>]					Print date (with specified format or %%c)\n");
	printf("\tkill <pid>						Kill process\n");
	printf("\tlibctest						Run libc-tests\n");
	printf("\tlogin							Just for fun ;)\n");
	printf("\tls [<dir>]						List current or specified directory\n");
	printf("\tmem								Print memory-usage\n");
	printf("\tprogress						Shows a progress-bar\n");
	printf("\tps								Print processes\n");
	printf("\techo <string>,...				Print given arguments\n");
	printf("\tenv [<name>|<name>=<value>]		Print or set env-variable(s)\n");
	printf("\thelp							Print this message\n");
	printf("\tcd <dir>						Change to given directory\n");
	printf("\tpwd								Print current directory\n");
	printf("\twc								Count and/or print words\n");

	printf("\n");
	printf("Additionally there is a basic shell-'language', that supports '|', '\"' and ';'\n");
	printf("So for example you can do:\n");
	printf("\tls; ps; echo \"test\";\n");
	printf("\tls | cat; ps\n");
	printf("\techo \"word1\" word2 \"word3 and 4\" | wc\n");
	printf("\tps | wc | wc -p\n");

	printf("\n");
	printf("Other:\n");
	printf("\tTab-Completion works for programs in /bin and files/directories\n");
	printf("\tYou can send EOF by CTRL+D and kill the current process with CTRL+C\n");
	printf("\tYou can scroll the screen with pageUp/-Down, shift + arrowUp/-Down\n");
	printf("\tYou can switch between vterm0 and vterm1 via CTRL+1/2\n");
	printf("\n");

	return 0;
}
