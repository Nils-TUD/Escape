/*
 * command.h -- command interpreter
 */


#ifndef _COMMAND_H_
#define _COMMAND_H_


void help(void);
char *exceptionToString(int exception);

Bool execCommand(char *line);


#endif /* _COMMAND_H_ */
