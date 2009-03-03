[BITS 32]

%include "syscalls.s"

[extern lastError]

SYSC_RET_1ARGS_ERR changeSize,SYSCALL_CHGSIZE
SYSC_RET_2ARGS_ERR _mapPhysical,SYSCALL_MAPPHYS
