[BITS 32]

%include "syscalls.s"

[extern lastError]

SYSC_RET_2ARGS_ERR setSigHandler,SYSCALL_SETSIGH
SYSC_RET_1ARGS_ERR unsetSigHandler,SYSCALL_UNSETSIGH
SYSC_VOID_1ARGS ackSignal,SYSCALL_ACKSIG
