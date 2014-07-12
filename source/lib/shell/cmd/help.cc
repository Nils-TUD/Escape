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

#include <sys/common.h>
#include <stdio.h>
#include "../cmds.h"

int shell_cmdHelp(A_UNUSED int argc,A_UNUSED char **argv) {
	printf("The shell provides a small scripting-language. It supports pipes," );
	printf("io-redirection, path-expansion, background-jobs, arithmetic, loops, ");
	printf("if-statements, functions and variables. So for example:\n");
	printf("	cat *.txt | wc > count.txt\n");
	printf("	ps -s cpu -n 10 &\n");
	printf("	$a := 2; if ($a > 2) then echo greater; else echo lesseq; fi\n");
	printf("	for($i := 0; $i < 10; $i := $i + 1) do echo ($i * 2); done\n");
	printf("	echo \"test ($a + `cat 'file.txt' | wc -c`) 123\"\n");
	printf("	function foo begin echo $0, $1; end; foo test\n");
	printf("	dump file.txt > dump.txt 2>&1\n");
	printf("	...\n");

	printf("\n");
	printf("Other shell features:\n");
	printf("	Tab-Completion works for programs in /bin and files/directories at the end of the line.\n");
	printf("	You can send EOF by CTRL+D and kill the current process with SIGINT via CTRL+C\n");
	printf("	You can scroll the screen with shift + up/down/pageUp/-Down\n");

	printf("\n");
	printf("UI-Manager features:\n");
	printf("	You can create a new text-UI via CTRL+T and a new graphical-UI via CTRL+G\n");
	printf("	You can use CTRL+LEFT/RIGHT or CTRL+0..7 to switch between UIs\n");
	printf("	You can enter the kernel-debugger via F12\n");
	return EXIT_SUCCESS;
}
