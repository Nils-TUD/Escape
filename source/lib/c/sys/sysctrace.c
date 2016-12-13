/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/log.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/sysctrace.h>
#include <sys/thread.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../stdio/iobuf.h"

struct Syscall {
	const char *name;
	const char *fmt;
};

const struct Syscall syscalls[] = {
	/* 0 */
	{"getpid",    		""							},
	{"getppid",    		"%d"						},
	{"debugc",    		"%c"						},
	{"fork",    		""							},
	{"exit",    		"%d"						},
	{"open",    		"%s,%O"						},
	{"close",    		"%d"						},
	{"read",    		"%d,%p,%x"					},
	{"createdev",    	"%d,%s,%x"					},
	{"chgsize",    		"%d"						},

	/* 10 */
	{"mmapphys",    	"%p,%x,%x,%x"				},
	{"write",    		"%d,%p,%x"					},
	{"yield",    		""							},
	{"dup",    			"%d"						},
	{"redirect",    	"%d,%d"						},
	{"signal",   		"%d,%p"						},
	{"acksignal",    	""							},
	{"kill",    		"%d,%d"						},
	{"exec",    		"%s,%p,%p"					},
	{"fcntl",    		"%d,%u,%d"					},

	/* 20 */
	{"init",	    	""							},
	{"sleep",    		"%u"						},
	{"seek",    		"%d,%u,%S"					},
	{"startthread",		"%p,%p"						},
	{"gettid",    		""							},
	{"send",    		"%d,%N,%p,%x"				},
	{"receive",    		"%d,%p,%p,%x"				},
	{"getcycles",    	""							},
	{"syncfs",    		"%d"						},
	{"link",    		"%d,%d,%s"					},

	/* 30 */
	{"unlink",    		"%d,%s"						},
	{"mkdir",    		"%d,%s"						},
	{"rmdir",    		"%d,%s"						},
	{"mount",    		"%d,%d,%s"					},
	{"unmount",    		"%d,%s"						},
	{"waitchild",    	"%p",						},
	{"tell",    		"%d,%p"						},
	{"sysconf",   		"%d"						},
	{"getwork",    		"%W,%p,%p,%x"				},
	{"join",    		"%d"						},

	/* 40 */
	{"fstat",    		"%d,%p"						},
	{"mmap",    		"%M"						},
	{"mprotect",    	"%p,%x"						},
	{"munmap",    		"%p"						},
	{"mattr",			"%p,%x,%x"					},
	{"getuid",    		""							},
	{"setuid",    		"%d"						},
	{"geteuid",    		""							},
	{"seteuid",    		"%d"						},
	{"getgid",    		""							},

	/* 50 */
	{"setgid",    		"%d"						},
	{"getegid",    		""							},
	{"setegid",   		"%d"						},
	{"chmod",    		"%d,%o"						},
	{"chown",    		"%d,%d,%d"					},
	{"getgroups",    	"%x,%p"						},
	{"setgroups",    	"%x,%p"						},
	{"isingroup",    	"%d,%d"						},
	{"alarm",    		"%u",						},
	{"tsctotime",    	"%u"						},

	/* 60 */
	{"semcrt",			"%u"						},
	{"semop",			"%d,%d"						},
	{"semdestr",		"%d"						},
	{"sendrecv",		"%d,%p,%p,%x"				},
	{"sharefile",		"%d,%p"						},
	{"cancel",			"%d,%u"						},
	{"creatsibl",		"%d,%d"						},
	{"sysconfstr",		"%d,%p,%x"					},
	{"clonems",			""							},
	{"joinms",			"%d"						},

	/* 70 */
	{"mlock",			"%p,%x"						},
	{"mlockall",		""							},
	{"semcrtirq",		"%d,%s,%p,%p"				},
	{"bindto",			"%d,%d"						},
	{"rename",			"%d,%s,%d,%s"				},
	{"gettimeofday",	"%p"						},
	{"utime",			"%d,%p"						},
	{"truncate",		"%d,%u"						},
#if defined(__x86__)
	{"reqports",   		"%d,%d"						},
	{"relports",    	"%d,%d"						},
#else
	{"debug",    		""							},
#endif
};

struct Messages {
	const char **msgs;
	size_t off;
	size_t count;
};

static const char *fileMsgs[] = {
	"FILE_OPEN",
	"FILE_READ",
	"FILE_WRITE",
	"FILE_SIZE",
	"FILE_CLOSE",
	"DEV_SHFILE",
	"DEV_CANCEL",
	"DEV_CREATSIBL",
};

static const char *fsMsgs[] = {
	"FS_OPEN",
	"FS_READ",
	"FS_WRITE",
	"FS_CLOSE",
	"FS_SYNCFS",
	"FS_LINK",
	"FS_UNLINK",
	"FS_RENAME",
	"FS_MKDIR",
	"FS_RMDIR",
	"FS_ISTAT",
	"FS_CHMOD",
	"FS_CHOWN",
	"FS_UTIME",
	"FS_TRUNCATE",
};

static const char *spkMsgs[] = {
	"SPEAKER_BEEP",
};

static const char *winMsgs[] = {
	"WIN_CREATE",
	"WIN_MOVE",
	"WIN_UPDATE",
	"WIN_DESTROY",
	"WIN_RESIZE",
	"WIN_ENABLE",
	"WIN_DISABLE",
	"WIN_ADDLISTENER",
	"WIN_REMLISTENER",
	"WIN_SET_ACTIVE",
	"WIN_ATTACH",
	"WIN_SETMODE",
	"WIN_EVENT",
};

static const char *scrMsgs[] = {
	"SCR_SETCURSOR",
	"SCR_GETMODE",
	"SCR_SETMODE",
	"SCR_GETMODES",
	"SCR_UPDATE",
};

static const char *vtMsgs[] = {
	"VT_GETFLAG",
	"VT_SETFLAG",
	"VT_BACKUP",
	"VT_RESTORE",
	"VT_SHELLPID",
	"VT_ISVTERM",
	"VT_SETMODE",
};

static const char *kbMsgs[] = {
	"KB_EVENT",
};

static const char *msMsgs[] = {
	"MS_EVENT",
};

static const char *uimMsgs[] = {
	"UIM_GETKEYMAP",
	"UIM_SETKEYMAP",
	"UIM_EVENT",
};

static const char *pciMsgs[] = {
	"PCI_GET_BY_CLASS",
	"PCI_GET_BY_ID",
	"PCI_GET_BY_INDEX",
	"PCI_GET_COUNT",
	"PCI_READ",
	"PCI_WRITE",
	"PCI_HAS_CAP",
	"PCI_ENABLE_MSIS",
};

static const char *initMsgs[] = {
	"INIT_REBOOT",
	"INIT_SHUTDOWN",
	"INIT_IAMALIVE",
};

static const char *nicMsgs[] = {
	"NIC_GETMAC",
	"NIC_GETMTU",
};

static const char *netMsgs[] = {
	"NET_LINK_ADD",
	"NET_LINK_REM",
	"NET_LINK_CONFIG",
	"NET_LINK_MAC",
	"NET_ROUTE_ADD",
	"NET_ROUTE_REM",
	"NET_ROUTE_CONFIG",
	"NET_ROUTE_GET",
	"NET_ARP_ADD",
	"NET_ARP_REM",
};

static const char *sockMsgs[] = {
	"SOCK_CONNECT",
	"SOCK_BIND",
	"SOCK_LISTEN",
	"SOCK_RECVFROM",
	"SOCK_SENDTO",
	"SOCK_ABORT",
};

static const char *dnsMsgs[] = {
	"DNS_RESOLVE",
	"DNS_SET_SERVER",
};

static const struct Messages msgs[] = {
	{fileMsgs,	50,		ARRAY_SIZE(fileMsgs)},
	{fsMsgs,	100,	ARRAY_SIZE(fsMsgs)},
	{spkMsgs,	200,	ARRAY_SIZE(spkMsgs)},
	{winMsgs,	300,	ARRAY_SIZE(winMsgs)},
	{scrMsgs,	400,	ARRAY_SIZE(scrMsgs)},
	{vtMsgs,	500,	ARRAY_SIZE(vtMsgs)},
	{kbMsgs,	600,	ARRAY_SIZE(kbMsgs)},
	{msMsgs,	700,	ARRAY_SIZE(msMsgs)},
	{uimMsgs,	800,	ARRAY_SIZE(uimMsgs)},
	{pciMsgs,	900,	ARRAY_SIZE(pciMsgs)},
	{initMsgs,	1000,	ARRAY_SIZE(initMsgs)},
	{nicMsgs,	1100,	ARRAY_SIZE(nicMsgs)},
	{netMsgs,	1200,	ARRAY_SIZE(netMsgs)},
	{sockMsgs,	1300,	ARRAY_SIZE(sockMsgs)},
	{dnsMsgs,	1400,	ARRAY_SIZE(dnsMsgs)},
};

struct OpenFlag {
	uint flag;
	const char *name;
};

static const struct OpenFlag openFlags[] = {
	{O_MSGS,	"O_MSGS"},
	{O_WRITE,	"O_WRITE"},
	{O_READ,	"O_READ"},
	{O_CREAT,	"O_CREAT"},
	{O_TRUNC,	"O_TRUNC"},
	{O_APPEND,	"O_APPEND"},
	{O_NONBLOCK,"O_NONBLOCK"},
	{O_LONELY,	"O_LONELY"},
	{O_EXCL,	"O_EXCL"},
	{O_NOCHAN,	"O_NOCHAN"},
};

static const char *seekModes[] = {
	"SET",
	"CUR",
	"END",
};

extern char __progname[];

static bool inTrace = false;

/* use -2 at first to call syscTraceEnter() for the first syscall. we'll check if it is enabled
 * there to avoid one additional check in every syscall, if it is disabled. */
int traceFd = -2;
static int syncFd = -1;
static bool validFd = false;

/* we have to be really careful here what functions we call. for example, we cannot use the heap,
 * because the heap performs syscalls and uses locking, so that we risk a deadlock when using
 * the heap here */

static void writeStr(const char *str,size_t length) {
	if(validFd) {
		A_UNUSED ssize_t res = write(traceFd,str,length);
	}
	else {
		for(size_t i = 0; i < length; ++i)
			logc(str[i]);
	}
}

static void writeTrace(FILE *os,bool enter,uint32_t *id) {
	char tmp[64];
	size_t length;
	char *buf = fgetbuf(os,&length);

	/* we want to do that atomically (multiple processes might write to the file) */
	fsemdown(syncFd);

	/* the idea of the sync protocol is as follows:
	 * - we increase a counter on every enter and leave
	 * - before enter and after leave, it should be even
	 * - to detect if somebody interrupted us between enter and leave, we store the counter in enter
	 *   and check if it is counter+1 in leave.
	 * - to detect if we interrupted someone else, we check if the counter is really even on enter.
	 *   if not, somebody else was interrupted after enter (before leave)
	 */
	uint32_t cnt;
	lseek(syncFd,0,SEEK_SET);
	sassert(read(syncFd,&cnt,sizeof(cnt)) == sizeof(cnt));

	if(validFd)
		lseek(traceFd,0,SEEK_END);

	if(enter) {
		/* if cnt is not even, somebody else did a system call enter, but no leave. */
		if(cnt % 2 != 0) {
			cnt++;
			writeStr("\n",1);
		}

		*id = cnt + 1;
	}
	else {
		/* were we interrupted between enter and leave? */
		if(cnt != *id) {
			/* maybe, the other one was interrupted, too */
			if(cnt % 2 == 1)
				writeStr("\n",1);

			snprintf(tmp,sizeof(tmp),"[%3d:%3d:%8.8s] ...",gettid(),getpid(),__progname);
			writeStr(tmp,strlen(tmp));
		}

		/* correct counter, if necessary */
		if(cnt % 2 != 1)
			cnt++;
	}

	writeStr(buf,length);

	cnt++;
	lseek(syncFd,0,SEEK_SET);
	sassert(write(syncFd,&cnt,sizeof(cnt)) == sizeof(cnt));

	fsemup(syncFd);
}

static void decodeMmap(FILE *os,struct mmap_params *p) {
	fprintf(os,"%p,%zu,%zu,%#x,%#x,%d,%lu",
		p->addr,p->length,p->loadLength,p->prot,p->flags,p->fd,p->offset);
}

static void decodeMsg(FILE *os,msgid_t id) {
	uint seq = id >> 16;
	uint mid = id & 0xFFFF;
	for(size_t i = 0; i < ARRAY_SIZE(msgs); ++i) {
		if(mid >= msgs[i].off && mid < msgs[i].off + msgs[i].count) {
			fprintf(os,"%#x:%s",seq,msgs[i].msgs[mid - msgs[i].off]);
			return;
		}
	}
	fprintf(os,"%#x:???",seq);
}

static void decodeOpen(FILE *os, uint flags) {
	bool first = true;
	for(size_t i = 0; i < ARRAY_SIZE(openFlags); ++i) {
		if(flags & openFlags[i].flag) {
			if(!first)
				fputs("|",os);
			fputs(openFlags[i].name,os);
			first = false;
		}
	}
}

static void decodeSeek(FILE *os, uint mode) {
	fprintf(os,"%s",seekModes[mode]);
}

void syscTraceEnter(long syscno,uint32_t *id,int argc,...) {
	/* we might perform syscalls ourself. thus, prevent recursion here */
	/* and ignore syscalls until the environment is initialized */
	if(inTrace || !environ)
		return;

	inTrace = true;

	/* not initialized yet? */
	if(traceFd == -2) {
		/* get fd to write the trace */
		char val[32];
		if(getenvto(val,sizeof(val),"SYSCTRC_FD") < 0)
			goto error;
		traceFd = atoi(val);
		validFd = fcntl(traceFd,F_GETFL,0) != -EBADF;

		/* get fd for synchronization */
		if(getenvto(val,sizeof(val),"SYSCTRC_SYNCFD") < 0)
			goto error;
		syncFd = atoi(val);
		assert(fcntl(syncFd,F_GETFL,0) != -EBADF);
	}

	{
		char tmp[256];
		FILE os;
		sassert(binit(&os,-1,O_WRONLY,tmp,0,sizeof(tmp) - 1,false));

		fprintf(&os,"[%3d:%3d:%8.8s] %s(",gettid(),getpid(),__progname,syscalls[syscno].name);

		va_list ap;
		va_start(ap,argc);
		int i = 0;
		const char *fmt = syscalls[syscno].fmt;
		while(*fmt) {
			if(*fmt != '%') {
				fputc(*fmt,&os);
				fmt++;
				continue;
			}

			assert(i < argc);
			ulong val = va_arg(ap,ulong);

			fmt++;
			switch(*fmt) {
				case 'd':
					fprintf(&os,"%ld",val);
					break;
				case 'u':
					fprintf(&os,"%lu",val);
					break;
				case 'o':
					fprintf(&os,"%#lo",val);
					break;
				case 'x':
					fprintf(&os,"%#lx",val);
					break;
				case 's':
					fprintf(&os,"%s",(char*)val);
					break;
				case 'p':
					fprintf(&os,"%p",(void*)val);
					break;
				case 'W':
					fprintf(&os,"%ld,%#lx",val >> 2,val & 0x3);
					break;
				case 'M':
					decodeMmap(&os,(struct mmap_params*)val);
					break;
				case 'N':
					decodeMsg(&os,val);
					break;
				case 'O':
					decodeOpen(&os,val);
					break;
				case 'S':
					decodeSeek(&os,val);
					break;
			}

			fmt++;
			i++;
		}
		fprintf(&os,")");

		writeTrace(&os,true,id);
	}

	inTrace = false;
	return;

error:
	inTrace = false;
	traceFd = -1;
}

void syscTraceLeave(uint32_t id,ulong res,long err) {
	if(inTrace || !environ)
		return;

	inTrace = true;

	char tmp[256];
	FILE os;
	sassert(binit(&os,-1,O_WRONLY,tmp,0,sizeof(tmp) - 1,false));
	fprintf(&os," = %#lx (%ld)\n",res,err);
	writeTrace(&os,false,&id);

	inTrace = false;
}
