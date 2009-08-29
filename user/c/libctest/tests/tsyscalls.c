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
#include <esc/service.h>
#include <esc/heap.h>
#include <esc/mem.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/fileio.h>
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
static void test_regService(void);
static void test_unregService(void);
static void test_changeSize(void);
static void test_mapPhysical(void);
static void test_write(void);
static void test_getClient(void);
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
static void test_createNode(void);
static void test_seek(void);
static void test_getFileInfo(void);

/* we want to be able to use u32 for all syscalls */
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
static s32 _regService(const char *name,u32 type) {
	return test_doSyscall(8,(u32)name,type,0);
}
static s32 _unregService(u32 id) {
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
static s32 _getClient(tServ *ids,u32 count,tServ *client) {
	return test_doSyscall(14,(u32)ids,count,(u32)client);
}
static s32 _requestIOPorts(u32 start,u32 count) {
	return test_doSyscall(15,start,count,0);
}
static s32 _releaseIOPorts(u32 start,u32 count) {
	return test_doSyscall(16,start,count,0);
}
static s32 _dupFd(u32 fd) {
	return test_doSyscall(17,fd,0,0);
}
static s32 _redirFd(u32 src,u32 dst) {
	return test_doSyscall(18,src,dst,0);
}
static s32 _wait(u32 ev) {
	return test_doSyscall(19,ev,0,0);
}
static s32 _setSigHandler(u32 sig,u32 handler) {
	return test_doSyscall(20,sig,handler,0);
}
static s32 _unsetSigHandler(u32 sig) {
	return test_doSyscall(21,sig,0,0);
}
static s32 _sendSignalTo(u32 pid,u32 sig,u32 data) {
	return test_doSyscall(23,pid,sig,data);
}
static s32 _exec(const char *path,const char **args) {
	return test_doSyscall(24,(u32)path,(u32)args,0);
}
static s32 _eof(u32 fd) {
	return test_doSyscall(25,fd,0,0);
}
static s32 _createNode(const char *path) {
	return test_doSyscall(28,(u32)path,0,0);
}
static s32 _seek(u32 fd,s32 pos,u32 whence) {
	return test_doSyscall(29,fd,pos,whence);
}
static s32 _getFileInfo(const char *path,sFileInfo *info) {
	return test_doSyscall(30,(u32)path,(u32)info,0);
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
	test_regService();
	test_unregService();
	test_changeSize();
	test_mapPhysical();
	test_write();
	test_getClient();
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
	test_createNode();
	test_seek();
	test_getFileInfo();
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
	test_assertInt(_open((char*)0,IO_READ),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open((char*)0xC0000000,IO_READ),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open((char*)0xFFFFFFFF,IO_READ),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open((char*)0x12345678,IO_READ),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open("abc",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open("abc",IO_READ << 4),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open(longName,IO_READ),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_open("",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("BLA",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("myfile",IO_READ),ERR_INVALID_PATH);
	test_assertInt(_open("/system/1234",IO_READ),ERR_VFS_NODE_NOT_FOUND);
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
	test_assertInt(_read(0,(void*)0x12345678,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0x12345678,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xC0000000,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,(void*)0xFFFFFFFF,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	/* this may be successfull if our stack has been resized previously and is now big enough */
	test_assertInt(_read(0,toosmallbuf,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(0,toosmallbuf,1024),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_read(-1,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_read(32,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_read(33,toosmallbuf,10),ERR_INVALID_FD);
	test_caseSucceded();
}

static void test_regService(void) {
	test_caseStart("Testing regService()");
	test_assertInt(_regService("MYVERYVERYVERYVERYVERYVERYVERYVERYLONGNAME",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("abc+-/",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("/",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService((char*)0xC0000000,0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService((char*)0xFFFFFFFF,0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("serv",SERV_DEFAULT << 4),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("serv",0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_regService("serv",0x12345678),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_unregService(void) {
	test_caseStart("Testing unregService()");
	test_assertInt(_unregService(0),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_unregService(12345678),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_unregService(0x7FFFFFFF),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_changeSize(void) {
	test_caseStart("Testing changeSize()");
	test_assertInt(_changeSize(-1000),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_changeSize(0xC0000000 / (4 * K)),ERR_NOT_ENOUGH_MEM);
	test_assertInt(_changeSize(0x7FFFFFFF),ERR_NOT_ENOUGH_MEM);
	test_caseSucceded();
}

static void test_mapPhysical(void) {
	test_caseStart("Testing mapPhysical()");
	/* try to map kernel */
	test_assertInt(__mapPhysical(0x100000,10),ERR_INVALID_SYSC_ARGS);
	test_assertInt(__mapPhysical(0x100123,4),ERR_INVALID_SYSC_ARGS);
	test_assertInt(__mapPhysical(0x100123,1231231231),ERR_INVALID_SYSC_ARGS);
	test_assertInt(__mapPhysical(0x100123,-1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(__mapPhysical(0x100123,-5),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_write(void) {
	char toosmallbuf[10];
	test_caseStart("Testing write()");
	test_assertInt(_write(0,(void*)0x12345678,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0x12345678,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xC0000000,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,4 * K + 1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,8 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,(void*)0xFFFFFFFF,8 * K - 1),ERR_INVALID_SYSC_ARGS);
	/* this may be successfull if our stack has been resized previously and is now big enough */
	test_assertInt(_write(0,toosmallbuf,4 * K),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(0,toosmallbuf,1024),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_write(-1,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(32,toosmallbuf,10),ERR_INVALID_FD);
	test_assertInt(_write(33,toosmallbuf,10),ERR_INVALID_FD);
	test_caseSucceded();
}

static void test_getClient(void) {
	tServ servs[3];
	tServ s;
	test_caseStart("Testing getClient()");
	/* test serv-array */
	test_assertInt(_getClient(NULL,1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0x12345678,1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0x12345678,4 * K,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0x12345678,4 * K + 1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0x12345678,8 * K,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0x12345678,8 * K - 1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xC0000000,1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,2,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,4 * K,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,4 * K + 1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,8 * K,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xBFFFFFFF,8 * K - 1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient((void*)0xFFFFFFFF,1,&s),ERR_INVALID_SYSC_ARGS);
	/* test client-id */
	test_assertInt(_getClient(servs,1,(tServ*)NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,1,(tServ*)0x12345678),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,1,(tServ*)0xC0000000),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,1,(tServ*)0xBFFFFFFF),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,1,(tServ*)0xFFFFFFFF),ERR_INVALID_SYSC_ARGS);
	/* test serv-array size */
	test_assertInt(_getClient(servs,-1,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,4 * K,&s),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getClient(servs,0x7FFFFFFF,&s),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_requestIOPorts(void) {
	test_caseStart("Testing requestIOPorts()");
	test_assertInt(_requestIOPorts(-1,-1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_requestIOPorts(-1,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_requestIOPorts(-1,3),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_requestIOPorts(1,-1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_requestIOPorts(1,0),ERR_INVALID_SYSC_ARGS);
	/* 0xF8 .. 0xFF is reserved */
	test_assertInt(_requestIOPorts(0xF8,1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8)),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8) - 1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8,(0xFF - 0xF8) + 1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,2),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,(0xFF - 0xF8)),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_requestIOPorts(0xF8 - 1,(0xFF - 0xF8) + 1),ERR_IO_MAP_RANGE_RESERVED);
	test_caseSucceded();
}

static void test_releaseIOPorts(void) {
	test_caseStart("Testing releaseIOPorts()");
	test_assertInt(_releaseIOPorts(-1,-1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_releaseIOPorts(-1,1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_releaseIOPorts(-1,3),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_releaseIOPorts(1,-1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_releaseIOPorts(1,0),ERR_INVALID_SYSC_ARGS);
	/* 0xF8 .. 0xFF is reserved */
	test_assertInt(_releaseIOPorts(0xF8,1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8)),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8) - 1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8,(0xFF - 0xF8) + 1),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,2),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,(0xFF - 0xF8)),ERR_IO_MAP_RANGE_RESERVED);
	test_assertInt(_releaseIOPorts(0xF8 - 1,(0xFF - 0xF8) + 1),ERR_IO_MAP_RANGE_RESERVED);
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
	test_assertInt(_wait(~(EV_CLIENT | EV_RECEIVED_MSG | EV_CHILD_DIED)),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_setSigHandler(void) {
	test_caseStart("Testing setSigHandler()");
	test_assertInt(_setSigHandler(-1,0),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(SIG_COUNT,0),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(0,0),ERR_INVALID_SIGNAL);
	test_assertInt(_setSigHandler(1,0xC0000000),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_setSigHandler(1,0xFFFFFFFF),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_setSigHandler(1,0x12345678),ERR_INVALID_SYSC_ARGS);
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
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	const char *a1[] = {"a",(const char*)0x12345678,NULL};
	const char *a2[] = {(const char*)0x12345678,NULL};
	const char *a3[] = {(const char*)0xC0000000,NULL};
	const char *a4[] = {(const char*)0xFFFFFFFF,NULL};
	const char *a5[] = {longPath,NULL};
	test_caseStart("Testing exec()");
	test_assertInt(_exec(NULL,NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec((const char*)0x12345678,NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec((const char*)0xC0000000,NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec((const char*)0xFFFFFFFF,NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec(longPath,NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec("p",a1),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec("p",a2),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec("p",a3),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec("p",a4),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_exec("p",a5),ERR_INVALID_SYSC_ARGS);
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

static void test_createNode(void) {
	char *longPath = (char*)malloc(10000);
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	test_caseStart("Testing createNode()");
	test_assertInt(_createNode(NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_createNode((const char*)0x12345678),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_createNode((const char*)0xC0000000),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_createNode((const char*)0xFFFFFFFF),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_createNode(longPath),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_createNode(""),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
	free(longPath);
}

static void test_seek(void) {
	test_caseStart("Testing seek()");
	test_assertInt(_seek(-1,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(-2,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0x7FFF,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(32,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(33,0,SEEK_SET),ERR_INVALID_FD);
	test_assertInt(_seek(0,-1,SEEK_SET),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
}

static void test_getFileInfo(void) {
	sFileInfo info;
	char *longPath = (char*)malloc(10000);
	memset(longPath,0x65,9999);
	longPath[9999] = 0;
	test_caseStart("Testing getFileInfo()");
	test_assertInt(_getFileInfo(NULL,&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo((const char*)0x12345678,&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo((const char*)0xC0000000,&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo((const char*)0xFFFFFFFF,&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo(longPath,&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo("",&info),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo("/bin",NULL),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo("/bin",(sFileInfo*)0x12345678),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo("/bin",(sFileInfo*)0xC0000000),ERR_INVALID_SYSC_ARGS);
	test_assertInt(_getFileInfo("/bin",(sFileInfo*)0xFFFFFFFF),ERR_INVALID_SYSC_ARGS);
	test_caseSucceded();
	free(longPath);
}
