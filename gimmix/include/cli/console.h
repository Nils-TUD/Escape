/**
 * $Id: console.h 154 2011-04-06 19:07:14Z nasmussen $
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "cli/lang/ast.h"

#define MAX_LINE_LEN	255

/**
 * Initializes the console and all commands
 */
void cons_init(void);

/**
 * Starts the console
 */
void cons_start(void);

/**
 * Stops the console
 */
void cons_stop(void);

/**
 * Executes the given command, if isFile is false. Otherwise it takes line as a filename and
 * executes all commands in that file
 *
 * @param line the command or filename
 * @param isFile whether line is a file or a command
 */
void cons_exec(const char *line,bool isFile);

/**
 * Displays the tokens found in the given command or file.
 *
 * @param line the command or filename
 * @param isFile whether line is a file or a command
 */
void cons_showTokens(const char *line,bool isFile);

/**
 * Sets the AST to use (called by the parser)
 *
 * @param t the AST
 */
void cons_setAST(sASTNode *t);

/**
 * Reads one character from given buffer (called by the lexer)
 *
 * @param buf the buffer
 * @return the character
 */
int cons_getc(char *buf);

/**
 * Reports the given error (called by the lexer/parser)
 *
 * @param s the message
 * @param ap the parameters
 */
void cons_error(const char *s,va_list ap);

/**
 * Performs the necessary actions for the current token (called by the lexer)
 */
void cons_tokenAction(void);

#endif /* CONSOLE_H_ */
