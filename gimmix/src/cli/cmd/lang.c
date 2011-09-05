/**
 * $Id: lang.c 165 2011-04-11 16:16:15Z nasmussen $
 */

#include "common.h"
#include "mmix/io.h"
#include "cli/cmds.h"
#include "cli/cmd/lang.h"

void cli_cmd_lang(size_t argc,const sASTNode **argv) {
	UNUSED(argc);
	UNUSED(argv);
	mprintf("Every command has the format:\n");
	mprintf("<cmdName> [<arg1> <arg2> ...]\n");
	mprintf("\n");
	mprintf("Each command-argument may be an arbitrary expression (some commands will"
			" limit it to a certain subset). You can use:\n");
	mprintf(" - Integer literals: 0x* or #* for hex, 0* for octal, 0b* for binary, * for decimal\n");
	mprintf(" - Floating point literals: e.g. 1.0, .6, 2.5e1, 12e-1, 1e+10\n");
	mprintf(" - Binary operators: +,-,*,s*,/,s/,%%,s%%,&,|,^,<<,>>,>>>\n");
	mprintf("   Notes: s*,s/ and s%% are the signed versions of the operations, >> shifts"
			" arithmetically, >>> shifts logically.\n");
	mprintf(" - Unary operators: -,~\n");
	mprintf(" - Ranges: x..y for x,x+1,...,y; x:y for x,x+1,...,x+y-1\n");
	mprintf(" - Special registers: rA,rB,...,rZ,rBB,rTT,rWW,rXX,rYY,rZZ,rSS\n");
	mprintf(" - @: The program counter\n");
	mprintf(" - Fetches: M[x],M1[x],M2[x],M4[x],M8[x] to get the contents at virt. address x\n");
	mprintf("              MX means: get X bytes, X-byte aligned.\n");
	mprintf("              Without X, it means 8 byte, not aligned.\n");
	mprintf("            m[x] to get the contents at phys. address x\n");
	mprintf("              Note that the IC and DC will be ignored. Use the privileged space with"
			" M[...] to respect the cache!\n");
	mprintf("            l[x],g[x],sp[x] to get the contents of local/global/special register x\n");
	mprintf("            $[x] or $x to get the contents of dynamic register x\n");
	mprintf("\n");
	mprintf("Arguments are separated by whitespace, i.e. you can't use whitespace in"
			" expressions! Note also, that you can (of course) combine everything in"
			" arbitrary ways, except that you can't nest ranges (\"1..2..3\" or similar).\n");
	mprintf("Additionally its worth mentioning that integers are converted to floats if one float"
			" occurs in a binary-operation. But note that floats don't support bit-operations"
			" and are not allowed in fetches!\n");
	mprintf("\n");
	mprintf("Some examples:\n");
	mprintf(" - p M4[@]        prints the 4 bytes at the PC\n");
	mprintf(" - p M[@:10]      prints the 10 bytes (actually 16) at the PC, in 8-byte packages\n");
	mprintf(" - p s M1[0..4]   prints the bytes at 0 to 4 in virtual memory as a string\n");
	mprintf(" - set $0 M[0]*2  sets $0 to the 8-bytes at 0, multiplied by 2\n");
	mprintf(" - d 0:12         disassembles the 3 4-byte instructions at these addresses\n");
	mprintf(" - p $[0..12]     prints the dynamic registers $0 to $12\n");
	mprintf(" - p x -1.4e-12   prints the hexadecimal integer-representation of the float\n");
}
