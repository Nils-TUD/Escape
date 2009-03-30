[BITS 32]

%include "syscalls.s"

[global cpu_rdtsc]

SYSC_VOID_1ARGS debugChar,SYSCALL_DEBUGCHAR

; u64 cpu_rdtsc(void);
cpu_rdtsc:
	rdtsc
	ret
