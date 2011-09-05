/**
 * $Id: console.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "cli/console.h"
#include "cli/lang/ast.h"
#include "cli/lang/eval.h"
#include "cli/cmds.h"
#include "core/cpu.h"
#include "getline/getline.h"
#include "lang/parser.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "exception.h"
#include "sim.h"
#include "event.h"

#define MAX_CMD_LEN		255
#define MAX_EXEC_ENVS	8

typedef struct {
	const char *filename;
	const char *line;
	FILE *stream;
	bool isStream;
	sASTNode *tree;
} sExecEnv;

extern void cons_showToken(int token);
static void cons_vterror(YYLTYPE t,const char *s,va_list ap);
static void cons_initParsing(const char *line,bool isFile);
static void cons_finishParsing(void);
static void cons_reset(void);

extern int yylex(void);
extern int yyparse(void);
extern int yylex_destroy(void);

extern YYLTYPE yylloc;
extern int yyleng;
extern int yylineno;
extern char *yytext;

// we need a stack here because exec() might execute a command that calls exec() again
static int curExecEnv;
static sExecEnv execEnvs[MAX_EXEC_ENVS];

static YYLTYPE yylastloc;
static int yycolumn;
static bool run = true;
static char lastCmd[MAX_CMD_LEN];

static void cons_sigIntrpt(const sEvArgs *args) {
	UNUSED(args);
	cmds_interrupt();
}

void cons_init(void) {
	curExecEnv = -1;
	ev_register(EV_USER_INT,cons_sigIntrpt);
	cmds_init();
}

void cons_start(void) {
	while(run) {
		char *line = gl_getline((char*)"GIMMIX > ");
		gl_histadd(line);
		if(!*line)
			break;
		// an empty line repeats the last command
		if(*line == '\n')
			line = lastCmd;
		cons_exec(line,false);
		strncpy(lastCmd,line,sizeof(lastCmd));
	}
}

void cons_stop(void) {
	run = false;
}

void cons_exec(const char *line,bool isFile) {
	cons_initParsing(line,isFile);

	int res = yyparse();
	// we need to reset the scanner if an error happened
	if(res != 0)
		yylex_destroy();
	else {
		bool finished = true;
		jmp_buf env;
		int ex = setjmp(env);
		if(ex == EX_NONE) {
			ex_push(&env);
			eval_get(execEnvs[curExecEnv].tree,0,&finished);
		}
		// if eval_get causes an exception, we get here and clean up
		ast_destroy(execEnvs[curExecEnv].tree);
		ex_pop();
	}

	cons_finishParsing();
}

void cons_showTokens(const char *line,bool isFile) {
	cons_initParsing(line,isFile);
	int token;
    do {
		token = yylex();
		cons_showToken(token);
	}
	while(token != 0);
    cons_finishParsing();
}

void cons_setAST(sASTNode *t) {
	execEnvs[curExecEnv].tree = t;
}

int cons_getc(char *buf) {
	int c;
	if(execEnvs[curExecEnv].isStream)
		c = getc(execEnvs[curExecEnv].stream);
	else
		c = *execEnvs[curExecEnv].line++;
	return (c == '\0' || c == EOF) ? 0 : (buf[0] = c, 1); \
}

void cons_tokenAction(void) {
	yylloc.filename = execEnvs[curExecEnv].filename;
	if(yylineno > 1 && yylloc.first_line != yylineno) {
		memcpy(&yylastloc,&yylloc,sizeof(YYLTYPE));
		yycolumn = 1;
		*yylloc.line = '\0';
	}
	yylloc.first_line = yylloc.last_line = yylineno;
	yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng - 1;
	yycolumn += yyleng;
	// '\n' is always alone
	if(*yytext != '\n')
		strncat(yylloc.line,yytext,MAX_LINE_LEN - strlen(yylloc.line));
}

void cons_error(const char *s,va_list ap) {
	if(strspn(yylloc.line," \t\n\r") == strlen(yylloc.line))
		cons_vterror(yylastloc,s,ap);
	else
		cons_vterror(yylloc,s,ap);
}

static void cons_vterror(YYLTYPE t,const char *s,va_list ap) {
	int i;
	if(t.first_line)
		fprintf(stderr,"%s:%d: ",t.filename,t.first_line);
	vfprintf(stderr,s,ap);
	fprintf(stderr,":\n");
	fprintf(stderr,"  %s\n",t.line);
	fprintf(stderr,"  %*s",t.first_column - 1,"");
	for(i = t.last_column - t.first_column; i >= 0; i--)
		putc('^',stderr);
	putc('\n',stderr);
}

static void cons_initParsing(const char *line,bool isFile) {
	curExecEnv++;
	assert(curExecEnv < MAX_EXEC_ENVS);
	execEnvs[curExecEnv].line = line;
	execEnvs[curExecEnv].isStream = isFile;
	if(isFile) {
		execEnvs[curExecEnv].stream = fopen(line,"r");
		if(execEnvs[curExecEnv].stream == NULL)
			sim_error("Unable to open file '%s'",line);
		execEnvs[curExecEnv].filename = line;
	}
	else
		execEnvs[curExecEnv].filename = "<stdin>";
	cons_reset();
}

static void cons_finishParsing(void) {
	if(execEnvs[curExecEnv].isStream)
		fclose(execEnvs[curExecEnv].stream);
	curExecEnv--;
}

static void cons_reset(void) {
	yylloc.filename = NULL;
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.last_line = 1;
	yylloc.last_column = 1;
	*yylloc.line = '\0';
	yylineno = 1;
	yycolumn = 1;
}
