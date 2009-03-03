[BITS 32]

%include "syscalls.s"

[extern lastError]
[global outb]
[global inb]

; void outb(u16 port,u8 val);
outb:
	mov		dx,[esp+4]										; load port
	mov		al,[esp+8]										; load value
	out		dx,al													; write to port
	ret

; u8 inb(u16 port);
inb:
	mov		dx,[esp+4]										; load port
	in		al,dx													; read from port
	ret

SYSC_RET_2ARGS_ERR requestIOPorts,SYSCALL_REQIOPORTS
SYSC_RET_2ARGS_ERR releaseIOPorts,SYSCALL_RELIOPORTS
SYSC_RET_2ARGS_ERR open,SYSCALL_OPEN
SYSC_RET_3ARGS_ERR read,SYSCALL_READ
SYSC_RET_3ARGS_ERR write,SYSCALL_WRITE
SYSC_RET_1ARGS_ERR sendEOT,SYSCALL_EOT
SYSC_VOID_1ARGS close,SYSCALL_CLOSE
