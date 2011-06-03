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
static void test_requestIOPorts(void);
static void test_releaseIOPorts(void);
static void test_dupFd(void);
static void test_redirFd(void);
static void test_wait(void);
static void test_setSigHandler(void);
static void test_sendSignalTo(void);
static void test_exec(void);
static void test_seek(void);
static void test_stat(void);

/* some convenience functions */
static int _getppidof(uint pid) {
	return doSyscall(SYSCALL_PPID,pid,0,0);
}
static int _open(const char *path,uint mode) {
	return doSyscall(SYSCALL_OPEN,(uint)path,mode,0);
}
static int _close(uint fd) {
	return doSyscall(SYSCALL_CLOSE,fd,0,0);
}
static int _read(uint fd,void *buffer,uint count) {
	return doSyscall(SYSCALL_READ,fd,(uint)buffer,count);
}
static int _regDriver(const char *name,uint type) {
	return doSyscall(SYSCALL_REG,(uint)name,type,0);
}
static int _changeSize(uint change) {
	return doSyscall(SYSCALL_CHGSIZE,change,0,0);
}
static int __mapPhysical(uint addr,uint count) {
	return doSyscall(SYSCALL_MAPPHYS,addr,count,0);
}
static int _write(uint fd,void *buffer,uint count) {
	return doSyscall(SYSCALL_WRITE,fd,(uint)buffer,count);
}
static int _requestIOPorts(uint start,uint count) {
	return doSyscall(SYSCALL_REQIOPORTS,start,count,0);
}
static int _releaseIOPorts(uint start,uint count) {
	return doSyscall(SYSCALL_RELIOPORTS,start,count,0);
}
static int _dupFd(uint fd) {
	return doSyscall(SYSCALL_DUPFD,fd,0,0);
}
static int _redirFd(uint src,uint dst) {
	return doSyscall(SYSCALL_REDIRFD,src,dst,0);
}
static int _wait(uint ev) {
	return doSyscall(SYSCALL_WAIT,ev,0,0);
}
static int _setSigHandler(uint sig,uint handler) {
	return doSyscall(SYSCALL_SETSIGH,sig,handler,0);
}
static int _sendSignalTo(uint pid,uint sig) {
	return doSyscall(SYSCALL_SENDSIG,pid,sig,0);
}
static int _exec(const char *path,const char **args) {
	return doSyscall(SYSCALL_EXEC,(uint)path,(uint)args,0);
}
static int _seek(uint fd,int pos,uint whence) {
	return doSyscall(SYSCALL_SEEK,fd,pos,whence);
}
static int _stat(const char *path,sFileInfo *info) {
	return doSyscall(SYSCALL_STAT,(uint)path,(uint)info,0);
}
static int _getWork(tFD *ids,uint count,tFD *client,tMsgId *mid,sMsg *msg,uint size,uint flags) {
	return doSyscall7(SYSCALL_GETWORK,(uint)ids,count,(uint)client,(uint)mid,(uint)msg,size,flags);
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
	test_wait();
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
	test_assertInt(_open((char*)0xC0000000,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open((char*)0xFFFFFFFF,IO_READ),ERR_INVALID_ARGS);
	test_assertInt(_open((char*)0x12345678,IO_READ),ERR_INVALID_ARGS);
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
	test_assertInt(_read(0,(void*)0x12345678,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,8 * 1024 - 1),ERR_INVALID_ARGS);
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
	test_assertInt(_regDriver((char*)0xC0000000,0),ERR_INVALID_ARGS);
	test_assertInt(_regDriver((char*)0xFFFFFFFF,0),ERR_INVALID_ARGS);
	test_assertInt(_regDriver("drv",0xFFFFFFFF),ERR_INVALID_ARGS);
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
	test_assertInt(_write(0,(void*)0x12345678,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,4 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,4 * 1024 + 1),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,8 * 1024),ERR_INVALID_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,8 * 1024 - 1),ERR_INVALID_ARGS);
	test_assertInt(_write(-1,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(32,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(33,toosmallbuf,10),ERR_INVALID_FD);
	test_caseSucceeded();
}

static void test_getWork(void) {
	tFD drvs[3];
	tFD s;
	tMsgId mid;
	sMsg msg;
	test_caseStart("Testing getWork()");
	/* test drv-array */
	test_assertInt(_getWork(NULL,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0x12345678,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0x12345678,4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0x12345678,4 * 1024 + 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0x12345678,8 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0x12345678,8 * 1024 - 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xC0000000,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,2,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,4 * 1024 + 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,8 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xBFFFFFFF,8 * 1024 - 1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork((void*)0xFFFFFFFF,1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test driver-id */
	test_assertInt(_getWork(drvs,1,(tFD*)0x12345678,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tFD*)0xC0000000,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tFD*)0xBFFFFFFF,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tFD*)0xFFFFFFFF,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test drv-array size */
	test_assertInt(_getWork(drvs,-1,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,4 * 1024,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,0x7FFFFFFF,&s,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test message-id */
	test_assertInt(_getWork(drvs,1,&s,(tMsgId*)0x12345678,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(tMsgId*)0xC0000000,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(tMsgId*)0xBFFFFFFF,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,(tMsgId*)0xFFFFFFFF,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	/* test message */
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)0x12345678,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)0xC0000000,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)0xBFFFFFFF,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,&s,&mid,(sMsg*)0xFFFFFFFF,sizeof(msg),0),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

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

static void test_wait(void) {
	test_caseStart("Testing wait()");
	test_assertInt(_wait(~(EV_CLIENT | EV_RECEIVED_MSG)),ERR_INVALID_ARGS);
	test_caseSucceeded();
}

static void test_setSigHandler(void) {
	test_caseStart("Testing setSigHandler()");
	test_assertInt(_setSigHandler(-4,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(SIG_COUNT,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(0,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(1,0xC0000000),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,0xFFFFFFFF),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,0x12345678),ERR_INVALID_ARGS);
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
	const char *a1[] = {"a",(const char*)0x12345678,NULL};
	const char *a2[] = {(const char*)0x12345678,NULL};
	const char *a3[] = {(const char*)0xC0000000,NULL};
	const char *a4[] = {(const char*)0xFFFFFFFF,NULL};
	const char *a5[] = {NULL,NULL};
	a5[0] = longPath;
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	test_caseStart("Testing exec()");
	test_assertInt(_exec(NULL,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)0x12345678,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)0xC0000000,NULL),ERR_INVALID_ARGS);
	test_assertInt(_exec((const char*)0xFFFFFFFF,NULL),ERR_INVALID_ARGS);
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
	test_assertInt(_stat((const char*)0x12345678,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat((const char*)0xC0000000,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat((const char*)0xFFFFFFFF,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat(longPath,&info),ERR_INVALID_ARGS);
	test_assertInt(_stat("",&info),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",NULL),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)0x12345678),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)0xC0000000),ERR_INVALID_ARGS);
	test_assertInt(_stat("/bin",(sFileInfo*)0xFFFFFFFF),ERR_INVALID_ARGS);
	test_caseSucceeded();
	free(longPath);
}
