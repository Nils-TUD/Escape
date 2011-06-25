/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/driver.h>
#include <esc/syscalls.h>
#include <esc/mem.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <esc/test.h>
#include <esc/conf.h>
#include <stdio.h>
#include <signal.h>
#include <errors.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tsyscalls.h"
#include "../syscalls.h"

/* forward declarations */
static void test_syscalls(void);
static void test_getppid(void);
static void test_open(void);
static void test_close(void);
static void test_read(void);
static void test_regDriver(void);
static void test_changeSize(void);
static void test_mapPhysical(void);
static void test_write(void);
static void test_getWork(void);
#ifdef __i386__
static void test_requestIOPorts(void);
static void test_releaseIOPorts(void);
#endif
static void test_dupFd(void);
static void test_redirFd(void);
static void test_setSigHandler(void);
static void test_sendSignalTo(void);
static void test_exec(void);
static void test_seek(void);
static void test_stat(void);

/* some convenience functions */
static int _getppidof(pid_t pid) {
	return doSyscall(SYSCALL_PPID,pid,0,0);
}
static int _open(const char *path,uint mode) {
	return doSyscall(SYSCALL_OPEN,(ulong)path,mode,0);
}
static int _close(int fd) {
	return doSyscall(SYSCALL_CLOSE,fd,0,0);
}
static int _read(int fd,void *buffer,size_t count) {
	return doSyscall(SYSCALL_READ,fd,(ulong)buffer,count);
}
static int _regDriver(const char *name,uint type) {
	return doSyscall(SYSCALL_REG,(ulong)name,type,0);
}
static int _changeSize(size_t change) {
	return doSyscall(SYSCALL_CHGSIZE,change,0,0);
}
static int __mapPhysical(uintptr_t addr,size_t count) {
	return doSyscall(SYSCALL_MAPPHYS,addr,count,0);
}
static int _write(int fd,void *buffer,size_t count) {
	return doSyscall(SYSCALL_WRITE,fd,(ulong)buffer,count);
}
#ifdef __i386__
static int _requestIOPorts(uint16_t start,size_t count) {
	return doSyscall(SYSCALL_REQIOPORTS,start,count,0);
}
static int _releaseIOPorts(uint16_t start,size_t count) {
	return doSyscall(SYSCALL_RELIOPORTS,start,count,0);
}
#endif
static int _dupFd(int fd) {
	return doSyscall(SYSCALL_DUPFD,fd,0,0);
}
static int _redirFd(int src,int dst) {
	return doSyscall(SYSCALL_REDIRFD,src,dst,0);
}
static int _setSigHandler(int sig,uintptr_t handler) {
	return doSyscall(SYSCALL_SETSIGH,sig,handler,0);
}
static int _sendSignalTo(pid_t pid,int sig) {
	return doSyscall(SYSCALL_SENDSIG,pid,sig,0);
}
static int _exec(const char *path,const char **args) {
	return doSyscall(SYSCALL_EXEC,(ulong)path,(ulong)args,0);
}
static int _seek(int fd,off_t pos,uint whence) {
	return doSyscall(SYSCALL_SEEK,fd,pos,whence);
}
static int _stat(const char *path,sFileInfo *info) {
	return doSyscall(SYSCALL_STAT,(ulong)path,(ulong)info,0);
}
static int _getWork(int *ids,size_t count,int *client,msgid_t *mid,sMsg *msg,size_t size,uint flags) {
	return doSyscall7(SYSCALL_GETWORK,(ulong)ids,count,(ulong)client,(ulong)mid,(ulong)msg,size,flags);
}

/* our test-module */
sTestModule tModSyscalls = {
	"Syscalls",
	&test_syscalls
};

static void test_syscalls(void) {
	test_getppid();
	test_open();
	test_close();
	test_read();
	test_regDriver();
	test_changeSize();
	test_mapPhysical();
	test_write();
	test_getWork();
#ifdef __i386__
	test_requestIOPorts();
	test_releaseIOPorts();
#endif
	test_dupFd();
	test_redirFd();
	test_setSigHandler();
	test_sendSignalTo();
	test_exec();
	test_seek();
	test_stat();
}

static void test_getppid(void) {
	test_caseStart("Testing getppid()");
	test_assertInt(_getppidof(-1),ERR_INVALID_PID);
	test_assertInt(_getppidof(-2),ERR_INVALID_PID);
	test_assertInt(_getppidof(1025),ERR_INVALID_PID);
	test_assertInt(_getppidof(1026),ERR_INVALID_PID);
	test_caseSucceeded();
}

static void test_open(void) {
	char *longName;
	test_caseStart("Testing open()");

	longName = (char*)malloc(10000);
	memset(longName,0x65,9999);
	longName[9999] = 0;
	test_assertInt(_open((char*)0,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open((char*)KERNEL_SPACE,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open((char*)HIGH_ADDR,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open((char*)ILLEGAL_ADDR,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open("abc",0),ERR_INVALID_ARGS);
	test_assertInt(_open("abc",IO_READ << 4),ERR_INVALID_ARGS);
	test_assertInt(_open(longName,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open("/system/1234",IO_READ),ERR_PATH_NOT_FOUND);
	free(longName);

	test_caseSucceeded();
}

static void test_close(void) {
	test_caseStart("Testing close()");
	_close(-1);
	_close(-2);
	_close(32);
	_close(33);
	test_caseSucceeded();
}

static void test_read(void) {
	char toosmallbuf[10];
	test_caseStart("Testing read()");
	test_assertInt(_read(0,(void*)ILLEGAL_ADDR,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)ILLEGAL_ADDR,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)ILLEGAL_ADDR,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)ILLEGAL_ADDR,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)ILLEGAL_ADDR,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)KERNEL_SPACE,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)KERNEL_SPACE,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)KERNEL_SPACE,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)KERNEL_SPACE,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)KERNEL_SPACE,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)HIGH_ADDR,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)HIGH_ADDR,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)HIGH_ADDR,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)HIGH_ADDR,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)HIGH_ADDR,8 * 1024 - 1),ERR_INVALID_ARGS);
	/* this may be successfull if our stack has been resized previously and is now big enough */
	test_assertInt(_read(0,toosmallbuf,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,toosmallbuf,1024),ERR_INVALID_ARGS);
	test_assertInt(_read(-1,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_read(32,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_read(33,toosmallbuf,10),ERR_INVALID_FD);
	test_caseSucceeded();
}

static void test_regDriver(void) {
	test_caseStart("Testing regDriver()");
	test_assertInt(_regDriver("MYVERYVERYVERYVERYVERYVERYVERYVERY"
			"VERYVERYVERYVERYVERYVERYVERYVERYVERYVERYVERYVERYVERYVERYLONGNAME",0),ERR_NOT_ENOUGH_MEM);
	test_assertInt(_regDriver("abc+-/",0),ERR_INV_DRIVER_NAME);
	test_assertInt(_regDriver("/",0),ERR_INV_DRIVER_NAME);
	test_assertInt(_regDriver("",0),ERR_INV_DRIVER_NAME);
	test_assertInt(_regDriver((char*)KERNEL_SPACE,0),ERR_INVALID_ARGS);
	test_assertInt(_regDriver((char*)HIGH_ADDR,0),ERR_INVALID_ARGS);
	test_assertInt(_regDriver("drv",(uint)HIGH_ADDR),ERR_INVALID_ARGS);
	test_assertInt(_regDriver("drv",DRV_READ | 1234),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

static void test_changeSize(void) {
	test_caseStart("Testing changeSize()");
	test_assertInt(_changeSize(-1000),ERR_NOT_ENOUGH_MEM);
	test_caseSucceeded();
}

static void test_mapPhysical(void) {
	test_caseStart("Testing mapPhysical()");
	/* try to map kernel */
	test_assertInt(__mapPhysical(0x100000,10),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,4),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,1231231231),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,-1),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,-5),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

static void test_write(void) {
	char toosmallbuf[10];
	test_caseStart("Testing write()");
	test_assertInt(_write(0,(void*)ILLEGAL_ADDR,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)ILLEGAL_ADDR,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)ILLEGAL_ADDR,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)ILLEGAL_ADDR,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)ILLEGAL_ADDR,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)KERNEL_SPACE,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)KERNEL_SPACE,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)KERNEL_SPACE,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)KERNEL_SPACE,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)KERNEL_SPACE,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)HIGH_ADDR,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)HIGH_ADDR,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)HIGH_ADDR,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)HIGH_ADDR,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)HIGH_ADDR,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(-1,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(32,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(33,toosmallbuf,10),ERR_INVALID_FD);
	test_caseSucceeded();
}

static void test_getWork(void) {
	int drvs[3];
	int s;
	msgid_t mid;
	sMsg msg;
	test_caseStart("Testing getWork()");
	/* test drv-array */
	test_assertInt(_getWork(NULL,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)ILLEGAL_ADDR,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)ILLEGAL_ADDR,4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)ILLEGAL_ADDR,4 * 1024 + 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)ILLEGAL_ADDR,8 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)ILLEGAL_ADDR,8 * 1024 - 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)KERNEL_SPACE,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),2,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),4 * 1024 + 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),8 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)(KERNEL_SPACE - 1),8 * 1024 - 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)HIGH_ADDR,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test driver-id */
	test_assertInt(_getWork(drvs,1,(int*)0x12345678,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(int*)KERNEL_SPACE,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(int*)(KERNEL_SPACE - 1),&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(int*)HIGH_ADDR,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test drv-array size */
	test_assertInt(_getWork(drvs,-1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,(KERNEL_SPACE - 1),&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test message-id */
	test_assertInt(_getWork(drvs,1,&s,(msgid_t*)0x12345678,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(msgid_t*)KERNEL_SPACE,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(msgid_t*)(KERNEL_SPACE - 1),&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(msgid_t*)HIGH_ADDR,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test message */
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)0x12345678,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)KERNEL_SPACE,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)(KERNEL_SPACE - 1),sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)HIGH_ADDR,sizeof(msg),0),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

#ifdef __i386__
static void test_requestIOPorts(void) {
	test_caseStart("Testing requestIOPorts()");
	test_assertInt(_requestIOPorts(-1,-1),ERR_INVALID_ARGS);
	test_assertInt(_requestIOPorts(-1,1),ERR_INVALID_ARGS);
	test_assertInt(_requestIOPorts(-1,3),ERR_INVALID_ARGS);
	test_assertInt(_requestIOPorts(1,-1),ERR_INVALID_ARGS);
	test_assertInt(_requestIOPorts(1,0),ERR_INVALID_ARGS);
	/* 0xF8 .. 0xFF is reserved */
	test_assertInt(_requestIOPorts(0xF8,1),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8)),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8) - 1),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8) + 1),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,2),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,(0xFF - 0xF8)),ERR_IOMAP_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,(0xFF - 0xF8) + 1),ERR_IOMAP_RESERVED);
	test_caseSucceeded();
}

static void test_releaseIOPorts(void) {
	test_caseStart("Testing releaseIOPorts()");
	test_assertInt(_releaseIOPorts(-1,-1),ERR_INVALID_ARGS);
	test_assertInt(_releaseIOPorts(-1,1),ERR_INVALID_ARGS);
	test_assertInt(_releaseIOPorts(-1,3),ERR_INVALID_ARGS);
	test_assertInt(_releaseIOPorts(1,-1),ERR_INVALID_ARGS);
	test_assertInt(_releaseIOPorts(1,0),ERR_INVALID_ARGS);
	/* 0xF8 .. 0xFF is reserved */
	test_assertInt(_releaseIOPorts(0xF8,1),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8)),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8) - 1),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8) + 1),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,2),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,(0xFF - 0xF8)),ERR_IOMAP_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,(0xFF - 0xF8) + 1),ERR_IOMAP_RESERVED);
	test_caseSucceeded();
}
#endif

static void test_dupFd(void) {
	test_caseStart("Testing dupFd()");
	test_assertInt(_dupFd(-1),ERR_INVALID_FD);
	test_assertInt(_dupFd(-2),ERR_INVALID_FD);
	test_assertInt(_dupFd(0x7FFF),ERR_INVALID_FD);
	test_assertInt(_dupFd(32),ERR_INVALID_FD);
	test_assertInt(_dupFd(33),ERR_INVALID_FD);
	test_caseSucceeded();
}

static void test_redirFd(void) {
	test_caseStart("Testing redirFd()");
	test_assertInt(_redirFd(-1,0),ERR_INVALID_FD);
	test_assertInt(_redirFd(-2,0),ERR_INVALID_FD);
	test_assertInt(_redirFd(0x7FFF,0),ERR_INVALID_FD);
	test_assertInt(_redirFd(32,0),ERR_INVALID_FD);
	test_assertInt(_redirFd(33,0),ERR_INVALID_FD);
	test_assertInt(_redirFd(0,-1),ERR_INVALID_FD);
	test_assertInt(_redirFd(0,-2),ERR_INVALID_FD);
	test_assertInt(_redirFd(0,0x7FFF),ERR_INVALID_FD);
	test_assertInt(_redirFd(0,32),ERR_INVALID_FD);
	test_assertInt(_redirFd(0,33),ERR_INVALID_FD);
	test_caseSucceeded();
}

static void test_setSigHandler(void) {
	size_t pageSize = getConf(CONF_PAGE_SIZE);
	test_caseStart("Testing setSigHandler()");
	test_assertInt(_setSigHandler(-4,pageSize),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(SIG_COUNT,pageSize),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(0,pageSize),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(1,KERNEL_SPACE),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,HIGH_ADDR),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,ILLEGAL_ADDR),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

static void test_sendSignalTo(void) {
	test_caseStart("Testing sendSignalTo()");
	test_assertInt(_sendSignalTo(0,-1),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_COUNT),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_ATA1),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_MOUSE),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_COM2),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(1024,0),ERR_INVALID_PID);
	test_assertInt(_sendSignalTo(1025,0),ERR_INVALID_PID);
	test_caseSucceeded();
}

static void test_exec(void) {
	char *longPath = (char*)malloc(10000);
	const char *a1[] = {"a",(const char*)ILLEGAL_ADDR,NULL};
	const char *a2[] = {(const char*)ILLEGAL_ADDR,NULL};
	const char *a3[] = {(const char*)KERNEL_SPACE,NULL};
	const char *a4[] = {(const char*)HIGH_ADDR,NULL};
	const char *a5[] = {NULL,NULL};
	a5[0] = longPath;
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	test_caseStart("Testing exec()");
	test_assertInt(_exec(NULL,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)ILLEGAL_ADDR,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)KERNEL_SPACE,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)HIGH_ADDR,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec(longPath,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec("p",a1),ERR_INVALID_ARGS);
	test_assertInt(_exec("p",a2),ERR_INVALID_ARGS);
	test_assertInt(_exec("p",a3),ERR_INVALID_ARGS);
	test_assertInt(_exec("p",a4),ERR_INVALID_ARGS);
	test_assertInt(_exec("p",a5),ERR_INVALID_ARGS);
	test_caseSucceeded();
	free(longPath);
}

static void test_seek(void) {
	test_caseStart("Testing seek()");
	test_assertInt(_seek(-1,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(-2,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0x7FFF,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(32,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(33,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0,-1,SEEK_SET),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

static void test_stat(void) {
	sFileInfo info;
	char *longPath = (char*)malloc(10000);
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	test_caseStart("Testing stat()");
	test_assertInt(_stat(NULL,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat((const char*)ILLEGAL_ADDR,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat((const char*)KERNEL_SPACE,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat((const char*)HIGH_ADDR,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat(longPath,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat("",&info),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",NULL),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)ILLEGAL_ADDR),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)KERNEL_SPACE),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)HIGH_ADDR),ERR_INVALID_ARGS);
	test_caseSucceeded();
	free(longPath);
}
