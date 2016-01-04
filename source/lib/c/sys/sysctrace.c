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
	{"open",    		"%s,%x"						},
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
	{"seek",    		"%d,%u"						},
	{"startthread",		"%p,%p"						},
	{"gettid",    		""							},
	{"send",    		"%d,%u,%p,%x"				},
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
