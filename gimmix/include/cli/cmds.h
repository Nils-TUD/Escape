/**
 * $Id: cmds.h 228 2011-06-11 20:02:45Z nasmussen $
 */

#ifndef CMDS_H_
#define CMDS_H_

#include "common.h"
#include "cli/lang/ast.h"
#include "cli/lang/eval.h"

#define MAX_VARIANTS	8

// the possible argument-types
#define ARGVAL_INT		(1 << 0)
#define ARGVAL_FLOAT	(1 << 1)
#define ARGVAL_STR		(1 << 2)
#define ARGVAL_ALL		(ARGVAL_INT | ARGVAL_FLOAT | ARGVAL_STR)

// command-interface
typedef void (*fCommand)(size_t argc,const sASTNode **argv);
typedef void (*fCmdInit)(void);
typedef void (*fCmdReset)(void);

// a command
typedef struct {
	const char *name;
	fCmdInit init;
	fCmdReset reset;
	fCommand cmd;
	struct {
		const char *synopsis;
		const char *desc;
	} variants[MAX_VARIANTS];
} sCommand;

/**
 * Initializes all commands
 */
void cmds_init(void);

/**
 * Resets all commands. It will be called if the machine is reset, so that all things that are
 * affected by it are reset. That means, breakpoints for example are NOT deleted and so on.
 */
void cmds_reset(void);

/**
 * Interrupts the current command as soon as possible
 */
void cmds_interrupt(void);

/**
 * Executes the command given by the AST-node-array. The first element is expected to be the
 * command-name, the rest are expected to be the arguments.
 *
 * @param node the AST-node-array
 */
void cmds_exec(const sASTNode *node);

/**
 * Prints the usage-information for the command with given name
 *
 * @param name the command-name (NULL=current)
 */
void cmds_printUsage(const char *name);

/**
 * Returns the command at given index
 *
 * @param index the index
 * @return the command or NULL
 */
const sCommand *cmds_get(size_t index);

/**
 * Evaluates the given argument. It will call eval_get() with given offset and will check whether
 * the type of the returned value is ok according to <expTypes>.
 *
 * @param arg the argument
 * @param expTypes the expected and allowed types
 * @param offset the offset to pass to eval_get()
 * @param finished will be set to true if all ranges are finished
 * @return the value of that argument
 */
sCmdArg cmds_evalArg(const sASTNode *arg,int expTypes,octa offset,bool *finished);

/**
 * Prints the given error and throws a CLI-exception. If <fmt> is NULL, it will print usage-
 * information for the current command.
 *
 * @param fmt the format
 */
void cmds_throwEx(const char *fmt,...);

#endif /* CMDS_H_ */
