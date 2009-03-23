[BITS 32]

%include "syscalls.s"

[extern lastError]
[global outByte]
[global outWord]
[global inByte]
[global inWord]

; void outByte(u16 port,u8 val);
outByte:
	mov		dx,[esp+4]										; load port
	mov		al,[esp+8]										; load value
	out		dx,al													; write to port
	ret

; void outw(u16 port,u16 val);
outWord:
	mov		dx,[esp+4]										; load port
	mov		ax,[esp+8]										; load value
	out		dx,ax													; write to port
	ret

; u8 inByte(u16 port);
inByte:
	mov		dx,[esp+4]										; load port
	in		al,dx													; read from port
	ret

; u16 inWord(u16 port);
inWord:
	mov		dx,[esp+4]										; load port
	in		ax,dx													; read from port
	ret

SYSC_RET_2ARGS_ERR requestIOPorts,SYSCALL_REQIOPORTS
SYSC_RET_2ARGS_ERR releaseIOPorts,SYSCALL_RELIOPORTS
SYSC_RET_2ARGS_ERR open,SYSCALL_OPEN
SYSC_RET_3ARGS_ERR read,SYSCALL_READ
SYSC_RET_1ARGS_ERR eof,SYSCALL_EOF
SYSC_RET_3ARGS_ERR write,SYSCALL_WRITE
SYSC_RET_1ARGS_ERR dupFd,SYSCALL_DUPFD
SYSC_RET_2ARGS_ERR redirFd,SYSCALL_REDIRFD
SYSC_VOID_1ARGS close,SYSCALL_CLOSE
