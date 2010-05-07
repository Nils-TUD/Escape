module tango.stdc.proc;

alias ushort pid_t;
alias ubyte sig_t;

const SIG_COUNT				= 18;

const SIG_KILL				= 0;
const SIG_TERM				= 1;
const SIG_ILL_INSTR			= 2;
const SIG_SEGFAULT			= 3;
const SIG_PROC_DIED			= 4;
const SIG_THREAD_DIED		= 5;
const SIG_CHILD_DIED		= 6;
const SIG_CHILD_TERM		= 7;
const SIG_INTRPT			= 8;
const SIG_INTRPT_TIMER		= 9;
const SIG_INTRPT_KB			= 10;
const SIG_INTRPT_COM1		= 11;
const SIG_INTRPT_COM2		= 12;
const SIG_INTRPT_FLOPPY		= 13;
const SIG_INTRPT_CMOS		= 14;
const SIG_INTRPT_ATA1		= 15;
const SIG_INTRPT_ATA2		= 16;
const SIG_INTRPT_MOUSE		= 17;

struct ExitState {
	pid_t pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	sig_t signal;
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
	extern int waitChild(ExitState *state);
	extern int sendSignal(sig_t signal,uint data);
	extern int sendSignalTo(pid_t pid,sig_t signal,uint data);
	extern int sleep(uint ms);
}
