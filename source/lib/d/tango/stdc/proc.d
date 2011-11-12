module tango.stdc.proc;

alias ushort pid_t;
alias ubyte int;

const SIG_COUNT				= 18;

const SIG_KILL				= 0;
const SIG_TERM				= 1;
const SIG_ILL_INSTR			= 2;
const SIG_SEGFAULT			= 3;
const SIG_PROC_DIED			= 4;
const SIG_PIPE_CLOSED		= 5;
const SIG_CHILD_TERM		= 6;
const SIG_INTRPT			= 7;
const SIG_INTRPT_TIMER		= 8;
const SIG_INTRPT_KB			= 9;
const SIG_INTRPT_COM1		= 10;
const SIG_INTRPT_COM2		= 11;
const SIG_INTRPT_FLOPPY		= 12;
const SIG_INTRPT_CMOS		= 13;
const SIG_INTRPT_ATA1		= 14;
const SIG_INTRPT_ATA2		= 15;
const SIG_INTRPT_MOUSE		= 16;

struct ExitState {
	pid_t pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	int signal;
	/* exit-code the process gave us via exit() */
	int exitCode;
	/* total amount of memory it has used */
	uint memory;
	/* cycle-count */
	ulong ucycleCount;
	ulong kcycleCount;
}

extern (C)
{
	extern int fork();
	extern int exec(char *path,char **args);
	extern int waitchild(ExitState *state);
	extern int kill(pid_t pid,int signal);
	extern int sleep(uint ms);
}
