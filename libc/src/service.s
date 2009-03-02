[BITS 32]

%include "syscalls.s"

[extern lastError]

SYSC_RET_1ARGS_ERR regService,SYSCALL_REG
SYSC_RET_1ARGS_ERR unregService,SYSCALL_UNREG
SYSC_RET_1ARGS_ERR waitForClient,SYSCALL_SWAIT
