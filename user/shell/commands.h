/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <common.h>

#define MAX_CMDNAME_LEN		30
#define MAX_CMD_LEN			40

#define TYPE_BUILTIN		0
#define TYPE_EXTERN			1
#define TYPE_PATH			2

#define APPS_DIR			"file:/apps/"

/* the builtin shell-commands */
typedef s32 (*fCommand)(u32 argc,s8 **argv);
typedef struct {
	u8 type;
	s8 name[MAX_CMDNAME_LEN + 1];
	fCommand func;
	s32 complStart;
} sShellCmd;

/**
 * Determines all matches for the given line
 *
 * @param line the line
 * @param length the current cursorpos
 * @param max the maximum number of matches to collect
 * @return the matches or NULL if failed
 */
sShellCmd **shell_getMatches(s8 *line,u32 length,u32 max);

/**
 * Free's the given matches
 *
 * @param matches the matches
 */
void shell_freeMatches(sShellCmd **matches);

#endif /* COMMANDS_H_ */
