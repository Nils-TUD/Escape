/*///////////////////////////////////////////////////////////////////////
 // $Id: gdbstub.cc,v 1.32 2008/02/15 19:03:53 sshwarts Exp $
 /////////////////////////////////////////////////////////////////////////
 //
 //  Copyright (C) 2002-2006  The Bochs Project Team
 //
 //  This library is free software; you can redistribute it and/or
 //  modify it under the terms of the GNU Lesser General Public
 //  License as published by the Free Software Foundation; either
 //  version 2 of the License, or (at your option) any later version.
 //
 //  This library is distributed in the hope that it will be useful,
 //  but WITHOUT ANY WARRANTY; without even the implied warranty of
 //  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 //  Lesser General Public License for more details.
 //
 //  You should have received a copy of the GNU Lesser General Public
 //  License along with this library; if not, write to the Free Software
 //  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

/*#define DEBUG*/

#ifdef DEBUG
#define GDB_DEBUG(str)		mprintf(str "\n")
#define GDB_DEBUGV(str,...)	mprintf(str "\n",## __VA_ARGS__)
#else
#define GDB_DEBUG(str)
#define GDB_DEBUGV(str,...)
#endif

#include "common.h"
#include "mmix/error.h"
#include "mmix/io.h"
#include "core/cpu.h"
#include "core/register.h"
#include "core/mmu.h"
#include "event.h"
#include "gdbstub.h"
#include "exception.h"
#include "sim.h"

#define NEED_CPU_REG_SHORTCUTS 1

#define GDBSTUB_STOP_NO_REASON			(0xac0)
#define GDBSTUB_EXECUTION_BREAKPOINT    (0xac1)
#define GDBSTUB_TRACE                   (0xac2)
#define GDBSTUB_USER_BREAK              (0xac3)

typedef unsigned long Bit32u;

static int last_stop_reason = GDBSTUB_STOP_NO_REASON;

static int listen_socket_fd;
static int socket_fd;

static int continue_thread = -1;
static int other_thread = 0;

#define NUMREGS (256 + 32 + 2)
#define NUMREGSBYTES (NUMREGS * 8)
static octa registers[NUMREGS];

#define MAX_BREAKPOINTS (255)
static octa breakpoints[MAX_BREAKPOINTS] = {0,};
static unsigned int nr_breakpoints = 0;

static octa instr_count = 0;

static char buf[4096],*bufptr = buf;
static const char hexchars[] = "0123456789abcdef";

static int hex(char ch) {
	if((ch >= 'a') && (ch <= 'f'))
		return (ch - 'a' + 10);
	if((ch >= '0') && (ch <= '9'))
		return (ch - '0');
	if((ch >= 'A') && (ch <= 'F'))
		return (ch - 'A' + 10);
	return (-1);
}

static octa switchBytes(octa in) {
	return (((in >> 56) & 0xFF) << 0) | (((in >> 48) & 0xFF) << 8) |
			(((in >> 40) & 0xFF) << 16) | (((in >> 32) & 0xFF) << 24) |
			(((in >> 24) & 0xFF) << 32) | (((in >> 16) & 0xFF) << 40) |
			(((in >> 8) & 0xFF) << 48) | (((in >> 0) & 0xFF) << 56);
}

static void flush_debug_buffer(void) {
	char *p = buf;
	while(p != bufptr) {
		int n = send(socket_fd,p,bufptr - p,0);
		if(n == -1) {
			sim_error(("error on debug socket"));
			break;
		}
		p += n;
	}
	bufptr = buf;
}

static void put_debug_char(char ch) {
	if(bufptr == buf + sizeof buf)
		flush_debug_buffer();
	*bufptr++ = ch;
}

static char get_debug_char(void) {
	char ch;
	recv(socket_fd,&ch,1,0);
	return (ch);
}

static void put_reply(const char *buffer) {
	unsigned char csum;
	int i;

	GDB_DEBUGV ("put_buffer %s", buffer);

	do {
		put_debug_char('$');

		csum = 0;

		i = 0;
		while(buffer[i] != 0) {
			put_debug_char(buffer[i]);
			csum = csum + buffer[i];
			i++;
		}

		put_debug_char('#');
		put_debug_char(hexchars[csum >> 4]);
		put_debug_char(hexchars[csum % 16]);
		flush_debug_buffer();
	}
	while(get_debug_char() != '+');
}

static void get_command(char *buffer) {
	unsigned char checksum;
	unsigned char xmitcsum;
	char ch;
	unsigned int count;
	unsigned int i;

	do {
		while((ch = get_debug_char()) != '$')
			;

		checksum = 0;
		xmitcsum = 0;
		count = 0;

		while(1) {
			ch = get_debug_char();
			if(ch == '#')
				break;
			checksum = checksum + ch;
			buffer[count] = ch;
			count++;
		}
		buffer[count] = 0;

		if(ch == '#') {
			xmitcsum = hex(get_debug_char()) << 4;
			xmitcsum += hex(get_debug_char());
			if(checksum != xmitcsum) {
				GDB_DEBUG ("Bad checksum");
			}
		}

		if(checksum != xmitcsum) {
			put_debug_char('-');
			flush_debug_buffer();
		}
		else {
			put_debug_char('+');
			if(buffer[2] == ':') {
				put_debug_char(buffer[0]);
				put_debug_char(buffer[1]);
				count = strlen(buffer);
				for(i = 3; i <= count; i++) {
					buffer[i - 3] = buffer[i];
				}
			}
			flush_debug_buffer();
		}
	}
	while(checksum != xmitcsum);
}

static void hex2mem(char *buffer,unsigned char *mem,int count) {
	int i;
	unsigned char ch;

	for(i = 0; i < count; i++) {
		ch = hex(*buffer++) << 4;
		ch = ch + hex(*buffer++);
		*mem = ch;
		mem++;
	}
}

static char *mem2hex(unsigned char *mem,char *buffer,int count) {
	int i;
	unsigned char ch;

	for(i = 0; i < count; i++) {
		ch = *mem;
		mem++;
		*buffer = hexchars[ch >> 4];
		buffer++;
		*buffer = hexchars[ch % 16];
		buffer++;
	}
	*buffer = 0;
	return (buffer);
}

static void evAfterExec(const sEvArgs *args) {
	UNUSED(args);
	unsigned int i;
	unsigned char ch;
	long arg;
	int r;

	instr_count++;

	if((instr_count % 500) == 0) {
		arg = fcntl(socket_fd,F_GETFL);
		fcntl(socket_fd,F_SETFL,arg | O_NONBLOCK);
		r = recv(socket_fd,&ch,1,0);
		fcntl(socket_fd,F_SETFL,arg);
		if(r == 1) {
			GDB_DEBUGV ("Got byte %x", (unsigned int)ch);
			last_stop_reason = GDBSTUB_USER_BREAK;
			cpu_pause();
			return;
		}
	}

	octa cpc = cpu_getPC();
	for(i = 0; i < nr_breakpoints; i++) {
		if(cpc == breakpoints[i]) {
			GDB_DEBUGV ("found breakpoint at %Ox", pc);
			last_stop_reason = GDBSTUB_EXECUTION_BREAKPOINT;
			cpu_pause();
			return;
		}
	}

	last_stop_reason = GDBSTUB_STOP_NO_REASON;
}

static int remove_breakpoint(octa addr) {
	unsigned int i;

	for(i = 0; i < MAX_BREAKPOINTS; i++) {
		if(breakpoints[i] == addr) {
			GDB_DEBUGV ("Removing breakpoint at %Ox", addr);
			breakpoints[i] = 0;
			return (1);
		}
	}
	return (0);
}

static void insert_breakpoint(octa addr) {
	unsigned int i;

	GDB_DEBUGV ("setting breakpoint at %Ox", addr);

	for(i = 0; i < (unsigned)MAX_BREAKPOINTS; i++) {
		if(breakpoints[i] == 0) {
			breakpoints[i] = addr;
			if(i >= nr_breakpoints) {
				nr_breakpoints = i + 1;
			}
			return;
		}
	}
	GDB_DEBUG ("No slot for breakpoint");
}

static void do_pc_breakpoint(int insert,octa addr) {
	if(insert)
		insert_breakpoint(addr);
	else
		remove_breakpoint(addr);
}

static void do_breakpoint(int insert,char *buffer) {
	char *ebuf;
	unsigned long type = strtoul(buffer,&ebuf,16);
	octa addr = mstrtoo(ebuf + 1,NULL,16);
	switch(type) {
		case 0:
		case 1:
			do_pc_breakpoint(insert,addr);
			put_reply("OK");
			break;
		default:
			put_reply("");
			break;
	}
}

static void write_signal(char *buffer,int sig) {
	buffer[0] = hexchars[sig >> 4];
	buffer[1] = hexchars[sig % 16];
	buffer[2] = 0;
}

static int access_mem(octa address,unsigned len,unsigned int rw,unsigned char *data) {
	int res = 1;
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE)
		res = 0;
	else {
		ex_push(&env);
		int i = 0;
		octa addr = address;
		while(len-- > 0) {
			if(rw)
				mmu_writeByte(addr,data[i++],0);
			else
				data[i++] = mmu_readByte(addr,0);
			addr++;
		}
	}
	ex_pop();
	return res;
}

static void debug_loop(void) {
	char buffer[255];
	char obuf[4096];
	int ne,i;
	unsigned char mem[255];

	ne = 0;

	while(ne == 0) {
		get_command(buffer);
		GDB_DEBUGV ("get_buffer %s", buffer);

		switch(buffer[0]) {
			case 'c': {
				octa new_eip;

				if(buffer[1] != 0) {
					new_eip = mstrtoo(buffer + 1,NULL,0);
					GDB_DEBUGV ("continuing at %x", new_eip);
					cpu_setPC(new_eip);
				}

				cpu_run();

				GDB_DEBUGV ("stopped with %x", last_stop_reason);
				obuf[0] = 'S';
				if(last_stop_reason == GDBSTUB_EXECUTION_BREAKPOINT || last_stop_reason
						== GDBSTUB_TRACE) {
					write_signal(&obuf[1],SIGTRAP);
				}
				else {
					write_signal(&obuf[1],0);
				}
				put_reply(obuf);
				break;
			}

			case 's': {
				GDB_DEBUG ("stepping");
				cpu_step();
				GDB_DEBUGV ("stopped with %x", last_stop_reason);
				obuf[0] = 'S';
				if(last_stop_reason == GDBSTUB_EXECUTION_BREAKPOINT || last_stop_reason
						== GDBSTUB_TRACE) {
					write_signal(&obuf[1],SIGTRAP);
				}
				else {
					write_signal(&obuf[1],SIGTRAP);
				}
				put_reply(obuf);
				break;
			}

			case 'M': {
				octa addr;
				int len;
				char *ebuf;

				addr = mstrtoo(&buffer[1],&ebuf,16);
				len = mstrtoo(ebuf + 1,&ebuf,16);
				hex2mem(ebuf + 1,mem,len);

				if (len == 1 && mem[0] == 0xcc)
				{
					insert_breakpoint(addr);
					put_reply("OK");
				}
				else if (remove_breakpoint(addr))
				{
					put_reply("OK");
				}
				else
				{
					if (access_mem(addr,len,1,mem))
					{
						put_reply("OK");
					}
					else
					{
						put_reply("Eff");
					}
				}
				break;
			}

			case 'm': {
				octa addr;
				int len;
				char *ebuf;

				addr = strtoull(&buffer[1],&ebuf,16);
				len = strtoul(ebuf + 1,NULL,16);
				GDB_DEBUGV ("addr %Lx len %x", addr, len);

				access_mem(addr,len,0,mem);
				mem2hex(mem,obuf,len);
				put_reply(obuf);
				break;
			}

			case 'P': {
				int reg;
				octa value;
				char* ebuf;

				reg = strtoul(&buffer[1],&ebuf,16);
				++ebuf;
				value = mstrtoo(ebuf,NULL,16);

				GDB_DEBUGV ("reg %d set to %Lx", reg, value);
				reg_set(reg,value);
				put_reply("OK");
				break;
			}

			case 'g':
				for(i = 0; i < 32; i++)
					registers[i] = switchBytes(reg_getSpecial(i));
				registers[32] = switchBytes(cpu_getPC());
				for(i = 0; i < 256; i++)
					registers[33 + i] = switchBytes(reg_get(255 - i));
				mem2hex((unsigned char*)registers,obuf,NUMREGSBYTES);
				put_reply(obuf);
				break;

			case '?':
				sprintf(obuf,"S%02x",SIGTRAP);
				put_reply(obuf);
				break;

			case 'H':
				if(buffer[1] == 'c') {
					continue_thread = strtol(&buffer[2],NULL,16);
					put_reply("OK");
				}
				else if(buffer[1] == 'g') {
					other_thread = strtol(&buffer[2],NULL,16);
					put_reply("OK");
				}
				else {
					put_reply("Eff");
				}
				break;

			case 'q':
				if(buffer[1] == 'C') {
					sprintf(obuf,"%lx",(Bit32u)1);
					put_reply(obuf);
				}
				else if(strncmp(&buffer[1],"Offsets",strlen("Offsets")) == 0) {
					/* TODO how to determine that? */
					sprintf(obuf,"Text=%x;Data=%x;Bss=%x",0,0,0);
					put_reply(obuf);
				}
				else {
					put_reply("Eff");
				}
				break;

			case 'Z':
				do_breakpoint(1,buffer + 1);
				break;
			case 'z':
				do_breakpoint(0,buffer + 1);
				break;
			case 'k':
				sim_error(("Debugger asked us to quit"));
				break;

			default:
				put_reply("");
				break;
		}
	}
}

static void wait_for_connect(int portn) {
	struct sockaddr_in sockaddr;
	socklen_t sockaddr_len;
	struct protoent *protoent;
	int r;
	int opt;

	listen_socket_fd = socket(PF_INET,SOCK_STREAM,0);
	if(listen_socket_fd == -1) {
		sim_error(("Failed to create socket"));
	}

	/* Allow rapid reuse of this port */
	opt = 1;
	r = setsockopt(listen_socket_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	if(r == -1) {
		GDB_DEBUG ("setsockopt(SO_REUSEADDR) failed");
	}

	memset(&sockaddr,'\000',sizeof sockaddr);
#if 0
	// if you don't have sin_len change that to #if 0.  This is the subject of
	// bug [ 626840 ] no 'sin_len' in 'struct sockaddr_in'.
	sockaddr.sin_len = sizeof sockaddr;
#endif
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(portn);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	r = bind(listen_socket_fd,(struct sockaddr *)&sockaddr,sizeof(sockaddr));
	if(r == -1) {
		sim_error(("Failed to bind socket"));
	}

	r = listen(listen_socket_fd,0);
	if(r == -1) {
		sim_error(("Failed to listen on socket"));
	}

	sockaddr_len = sizeof sockaddr;
	socket_fd = accept(listen_socket_fd,(struct sockaddr *)&sockaddr,&sockaddr_len);
	if(socket_fd == -1) {
		sim_error(("Failed to accept on socket"));
	}
	close(listen_socket_fd);

	protoent = getprotobyname("tcp");
	if(!protoent) {
		GDB_DEBUG ("getprotobyname (\"tcp\") failed");
		return;
	}

	/* Disable Nagle - allow small packets to be sent without delay. */
	opt = 1;
	r = setsockopt(socket_fd,protoent->p_proto,TCP_NODELAY,&opt,sizeof(opt));
	if(r == -1) {
		GDB_DEBUG ("setsockopt(TCP_NODELAY) failed");
	}
	unsigned long ip = sockaddr.sin_addr.s_addr;
	mprintf("Connected to %d.%d.%d.%d\n",(int)(ip & 0xff),(int)((ip >> 8) & 0xff),(int)((ip >> 16)
			& 0xff),(int)((ip >> 24) & 0xff));
}

void gdbstub_init(int portn) {
	/* Wait for connect */
	mprintf("Waiting for gdb connection on port %d\n",portn);
	wait_for_connect(portn);

	ev_register(EV_AFTER_EXEC,evAfterExec);

	/* Do debugger command loop */
	debug_loop();
}
