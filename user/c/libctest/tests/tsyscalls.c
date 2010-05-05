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
#include <messages.h>
#include <esc/driver.h>
#include <esc/heap.h>
#include <esc/mem.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <stdio.h>
#include <esc/signals.h>
#include <esc/ports.h>
#include <errors.h>
#include <test.h>
#include <string.h>
#include <stdarg.h>
#include "tsyscalls.h"

/* forward declarations */
static void test_syscalls(void);
static void test_getppid(void);
static void test_open(void);
static void test_close(void);
static void test_read(void);
static void test_regDriver(void);
static void test_unregDriver(void);
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
static void test_unsetSigHandler(void);
static void test_sendSignalTo(void);
static void test_exec(void);
static void test_eof(void);
static void test_seek(void);
static void test_stat(void);

/* we want to be able to use u32 for all syscalls */
static s32 test_doSyscall7(u32 syscallNo,u32 arg1,u32 arg2,u32 arg3,u32 arg4,u32 arg5,u32 arg6,u32 arg7) {
	s32 res;
	__asm__ __volatile__ (
		"movl	%2,%%ecx\n"
		"movl	%3,%%edx\n"
		"movl	%7,%%eax\n"
		"pushl	%%eax\n"
		"movl	%6,%%eax\n"
		"pushl	%%eax\n"
		"movl	%5,%%eax\n"
		"pushl	%%eax\n"
		"movl	%4,%%eax\n"
		"pushl	%%eax\n"
		"movl	%1,%%eax\n"
		"int	$0x30\n"
		"add	$16,%%esp\n"
		"test	%%ecx,%%ecx\n"
		"jz		1f\n"
		"movl	%%ecx,%%eax\n"
		"1:\n"
		"mov	%%eax,%0\n"
		: "=a" (res) : "m" (syscallNo), "m" (arg1), "m" (arg2), "m" (arg3), "m" (arg4),
		  "m" (arg5), "m" (arg6), "m" (arg7)
	);
	return res;
}
static s32 test_doSyscall(u32 syscallNo,u32 arg1,u32 arg2,u32 arg3) {
	s32 res;
	__asm__ __volatile__ (
		"movl	%2,%%ecx\n"
		"movl	%3,%%edx\n"
		"movl	%4,%%eax\n"
		"pushl	%%eax\n"
		"movl	%1,%%eax\n"
		"int	$0x30\n"
		"add	$4,%%esp\n"
		"test	%%ecx,%%ecx\n"
		"jz		1f\n"
		"movl	%%ecx,%%eax\n"
		"1:\n"
		"mov	%%eax,%0\n"
		: "=a" (res) : "m" (syscallNo), "m" (arg1), "m" (arg2), "m" (arg3)
	);
	return res;
}
/* some convenience functions */
static s32 _getppidof(u32 pid) {
	return test_doSyscall(1,pid,0,0);
}
static s32 _open(const char *path,u32 mode) {
	return test_doSyscall(5,(u32)path,mode,0);
}
static s32 _close(u32 fd) {
	return test_doSyscall(6,fd,0,0);
}
static s32 _read(u32 fd,void *buffer,u32 count) {
	return test_doSyscall(7,fd,(u32)buffer,count);
}
static s32 _regDriver(const char *name,u32 type) {
	return test_doSyscall(8,(u32)name,type,0);
}
static s32 _unregDriver(u32 id) {
	return test_doSyscall(9,id,0,0);
}
static s32 _changeSize(u32 change) {
	return test_doSyscall(10,change,0,0);
}
static s32 __mapPhysical(u32 addr,u32 count) {
	return test_doSyscall(11,addr,count,0);
}
static s32 _write(u32 fd,void *buffer,u32 count) {
	return test_doSyscall(12,fd,(u32)buffer,count);
}
static s32 _getWork(tDrvId *ids,u32 count,tDrvId *client,tMsgId *mid,sMsg *msg,u32 size,u8 flags) {
	return test_doSyscall7(57,(u32)ids,count,(u32)client,(u32)mid,(u32)msg,size,flags);
}
static s32 _requestIOPorts(u32 start,u32 count) {
	return test_doSyscall(14,start,count,0);
}
static s32 _releaseIOPorts(u32 start,u32 count) {
	return test_doSyscall(15,start,count,0);
}
static s32 _dupFd(u32 fd) {
	return test_doSyscall(16,fd,0,0);
}
static s32 _redirFd(u32 src,u32 dst) {
	return test_doSyscall(17,src,dst,0);
}
static s32 _wait(u32 ev) {
	return test_doSyscall(18,ev,0,0);
}
static s32 _setSigHandler(u32 sig,u32 handler) {
	return test_doSyscall(19,sig,handler,0);
}
static s32 _unsetSigHandler(u32 sig) {
	return test_doSyscall(20,sig,0,0);
}
static s32 _sendSignalTo(u32 pid,u32 sig,u32 data) {
	return test_doSyscall(22,pid,sig,data);
}
static s32 _exec(const char *path,const char **args) {
	return test_doSyscall(23,(u32)path,(u32)args,0);
}
static s32 _eof(u32 fd) {
	return test_doSyscall(24,fd,0,0);
}
static s32 _seek(u32 fd,s32 pos,u32 whence) {
	return test_doSyscall(27,fd,pos,whence);
}
static s32 _stat(const char *path,sFileInfo *info) {
	return test_doSyscall(28,(u32)path,(u32)info,0);
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
	test_unregDriver();
	test_changeSize();
	test_mapPhysical();
	test_write();
	test_getWork();
	test_requestIOPorts();
	test_releaseIOPorts();
	test_dupFd();
	test_redirFd();
	test_wait();
	test_setSigHandler();
	test_unsetSigHandler();
	test_sendSignalTo();
	test_exec();
	test_eof();
	test_seek();
	test_stat();
}

static void test_getppid(void) {
	test_caseStart("Testing getppid()");
	test_assertInt(_getppidof(-1),ERR_INVALID_PID);
	test_assertInt(_getppidof(-2),ERR_INVALID_PID);
	test_assertInt(_getppidof(1025),ERR_INVALID_PID);
	test_assertInt(_getppidof(1026),ERR_INVALID_PID);
	test_caseSucceded();
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
	test_assertInt(_open("",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("BLA",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("myfile",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("/system/1234",IO_READ),ERR_PATH_NOT_FOUND);
	free(longName);

	test_caseSucceded();
}

static void test_close(void) {
	test_caseStart("Testing close()");
	_close(-1);
	_close(-2);
	_close(32);
	_close(33);
	test_caseSucceded();
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
	test_caseSucceded();
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
	test_caseSucceded();
}

static void test_unregDriver(void) {
	test_caseStart("Testing unregDriver()");
	test_assertInt(_unregDriver(-1),ERR_INVALID_ARGS);
	test_assertInt(_unregDriver(12345678),ERR_INVALID_ARGS);
	test_assertInt(_unregDriver(0x7FFFFFFF),ERR_INVALID_ARGS);
	test_caseSucceded();
}

static void test_changeSize(void) {
	test_caseStart("Testing changeSize()");
	test_assertInt(_changeSize(-1000),ERR_NOT_ENOUGH_MEM);
	test_assertInt(_changeSize(0xC0000000 / (4 * 1024)),ERR_NOT_ENOUGH_MEM);
	test_assertInt(_changeSize(0x7FFFFFFF),ERR_NOT_ENOUGH_MEM);
	test_caseSucceded();
}

static void test_mapPhysical(void) {
	test_caseStart("Testing mapPhysical()");
	/* try to map kernel */
	test_assertInt(__mapPhysical(0x100000,10),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,4),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,1231231231),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,-1),ERR_INVALID_ARGS);
	test_assertInt(__mapPhysical(0x100123,-5),ERR_INVALID_ARGS);
	test_caseSucceded();
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
	test_caseSucceded();
}

static void test_getWork(void) {
	tDrvId drvs[3];
	tDrvId s;
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
	test_assertInt(_getWork(drvs,1,(tDrvId*)0x12345678,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tDrvId*)0xC0000000,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tDrvId*)0xBFFFFFFF,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
	test_assertInt(_getWork(drvs,1,(tDrvId*)0xFFFFFFFF,&mid,&msg,sizeof(msg),0),ERR_INVALID_ARGS);
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
	test_caseSucceded();
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
	test_caseSucceded();
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
	test_caseSucceded();
}

static void test_dupFd(void) {
	test_caseStart("Testing dupFd()");
	test_assertInt(_dupFd(-1),ERR_INVALID_FD);
	test_assertInt(_dupFd(-2),ERR_INVALID_FD);
	test_assertInt(_dupFd(0x7FFF),ERR_INVALID_FD);
	test_assertInt(_dupFd(32),ERR_INVALID_FD);
	test_assertInt(_dupFd(33),ERR_INVALID_FD);
	test_caseSucceded();
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
	test_caseSucceded();
}

static void test_wait(void) {
	test_caseStart("Testing wait()");
	test_assertInt(_wait(~(EV_CLIENT | EV_RECEIVED_MSG)),ERR_INVALID_ARGS);
	test_caseSucceded();
}

static void test_setSigHandler(void) {
	test_caseStart("Testing setSigHandler()");
	test_assertInt(_setSigHandler(-1,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(SIG_COUNT,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(0,0x1000),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(1,0xC0000000),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,0xFFFFFFFF),ERR_INVALID_ARGS);
	test_assertInt(_setSigHandler(1,0x12345678),ERR_INVALID_ARGS);
	test_caseSucceded();
}

static void test_unsetSigHandler(void) {
	test_caseStart("Testing unsetSigHandler()");
	test_assertInt(_unsetSigHandler(-1),ERR_INVALID_SIGNAL);
	test_assertInt(_unsetSigHandler(SIG_COUNT),ERR_INVALID_SIGNAL);
	test_assertInt(_unsetSigHandler(0),ERR_INVALID_SIGNAL);
	test_caseSucceded();
}

static void test_sendSignalTo(void) {
	test_caseStart("Testing sendSignalTo()");
	test_assertInt(_sendSignalTo(0,-1,0),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_COUNT,0),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_ATA1,0),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_MOUSE,0),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(0,SIG_INTRPT_COM2,0),ERR_INVALID_SIGNAL);
	test_assertInt(_sendSignalTo(-1,0,0),ERR_INVALID_PID);
	test_assertInt(_sendSignalTo(1024,0,0),ERR_INVALID_PID);
	test_assertInt(_sendSignalTo(1025,0,0),ERR_INVALID_PID);
	test_caseSucceded();
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
	test_caseSucceded();
	free(longPath);
}

static void test_eof(void) {
	test_caseStart("Testing eof()");
	test_assertInt(_eof(-1),ERR_INVALID_FD);
	test_assertInt(_eof(-2),ERR_INVALID_FD);
	test_assertInt(_eof(0x7FFF),ERR_INVALID_FD);
	test_assertInt(_eof(32),ERR_INVALID_FD);
	test_assertInt(_eof(33),ERR_INVALID_FD);
	test_caseSucceded();
}

static void test_seek(void) {
	test_caseStart("Testing seek()");
	test_assertInt(_seek(-1,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(-2,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0x7FFF,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(32,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(33,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0,-1,SEEK_SET),ERR_INVALID_ARGS);
	test_caseSucceded();
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
	test_caseSucceded();
	free(longPath);
}
