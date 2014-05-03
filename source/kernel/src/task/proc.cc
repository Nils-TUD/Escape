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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/groups.h>
#include <sys/task/filedesc.h>
#include <sys/task/timer.h>
#include <sys/task/sems.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/physmem.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/useraccess.h>
#include <sys/interrupts.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/fs.h>
#include <sys/vfs/openfile.h>
#include <sys/spinlock.h>
#include <sys/mutex.h>
#include <sys/util.h>
#include <sys/syscalls.h>
#include <sys/log.h>
#include <sys/term.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#define DEBUG_CREATIONS					0
/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

/* we have to put the first one here because we need a process before we can allocate
 * something on the kernel-heap. sounds strange, but the kheap-init stuff uses paging and
 * this wants to have the current process. So, I guess the cleanest way is to simply put
 * the first process in the data-area (we can't free the first one anyway) */
Proc ProcBase::first;
/* our processes */
SList<Proc> ProcBase::procs;
Proc *ProcBase::pidToProc[MAX_PROC_COUNT];
pid_t ProcBase::nextPid = 1;
Mutex ProcBase::procLock;
Mutex ProcBase::childLock;
SpinLock ProcBase::refLock;

Proc *ProcBase::getRef(pid_t pid) {
	if(pid >= ARRAY_SIZE(pidToProc))
		return NULL;

	LockGuard<SpinLock> guard(&refLock);
	Proc *p = pidToProc[pid];
	if(p)
		p->refs++;
	return p;
}

void ProcBase::relRef(const Proc *p) {
	bool lastRef;
	{
		LockGuard<SpinLock> guard(&refLock);
		if((lastRef = (--p->refs == 0)))
			remove(const_cast<Proc*>(p));
	}
	if(lastRef) {
		/* now, nobody can get the process anymore, so we can destroy everything without worrying */
		Cache::free((char*)p->command);
		Cache::free(const_cast<Proc*>(p));
	}
}

void ProcBase::init() {
	/* init the first process */
	Proc *p = &first;

	p->pid = 0;
	p->parentPid = 0;
	p->ruid = ROOT_UID;
	p->euid = ROOT_UID;
	p->suid = ROOT_UID;
	p->rgid = ROOT_GID;
	p->egid = ROOT_GID;
	p->sgid = ROOT_GID;
	p->groups = NULL;
	p->sigRetAddr = 0;
	p->flags = 0;
	p->entryPoint = 0;
	p->priority = MAX_PRIO;
	p->initProps();
	MountSpace::create(p);
	VFS::mountAll(p);
	if(Sems::init(p) < 0)
		Util::panic("Unable to init semaphores");
	memclear(p->locks,sizeof(p->locks));
	for(size_t i = 0; i < PMUTEX_COUNT; ++i)
		p->mutexes[i] = Mutex();
	p->command = strdup("initloader");
	/* create nodes in vfs */
	p->threadsDir = VFS::createProcess(p->pid);
	if(p->threadsDir < 0)
		Util::panic("Not enough mem for init process");

	/* init fds */
	if(FileDesc::init(p) < 0)
		Util::panic("Not enough mem to init file-descriptors of init-process");

	/* create first thread */
	if(!p->threads.append(Thread::init(p)))
		Util::panic("Unable to append the initial thread");

	/* init virt mem */
	p->virtmem.init(p);

	/* add to procs */
	add(p);
}

void ProcBase::initProps() {
	stats.input = 0;
	stats.output = 0;
	stats.totalRuntime = 0;
	stats.lastCycles = 0;
	stats.totalSyscalls = 0;
	stats.totalScheds = 0;
	stats.totalMigrations = 0;
	stats.exitCode = 0;
	stats.exitSignal = SIG_COUNT;
	threads = ISList<Thread*>();
	refs = 1;
}

const char *ProcBase::getProgram() const {
	static char buffer[32];
	const char *cmd = command;
	size_t i;
	for(i = 0; i < sizeof(buffer) - 1 && *cmd && *cmd != ' '; ++i)
		buffer[i] = *cmd++;
	buffer[i] = '\0';
	return buffer;
}

void ProcBase::setCommand(const char *cmd,int argc,const char *args) {
	size_t cmdlen = strlen(cmd);
	size_t len = cmdlen + 1;
	const char *curargs = args;
	if(command)
		Cache::free((char*)command);

	/* determine total length */
	for(int i = 0; i < argc; ++i) {
		size_t curlen = strlen(curargs) + 1;
		if(i > 0)
			len += curlen;
		curargs += curlen;
	}

	/* copy cmd and arguments into buffer */
	command = (char*)Cache::alloc(len);
	char *curdst = (char*)command;
	memcpy(curdst,cmd,cmdlen);
	curdst += cmdlen;
	for(int i = 0; i < argc; ++i) {
		size_t curlen = strlen(args);
		if(i > 0) {
			if(i == 1)
				*curdst++ = ' ';
			memcpy(curdst,args,curlen);
			curdst += curlen;
			if(i < argc - 1)
				*curdst++ = ' ';
		}
		args += curlen + 1;
	}
	*curdst = '\0';
}

void ProcBase::getMemUsageOf(pid_t pid,size_t *own,size_t *shared,size_t *swapped) {
	Proc *p = request(pid,PLOCK_PROG);
	if(p) {
		*own = p->virtmem.getOwnFrames();
		*shared = p->virtmem.getSharedFrames();
		*swapped = p->virtmem.getSwappedFrames();
		*own = *own + p->getKMemUsage();
		release(p,PLOCK_PROG);
	}
}

void ProcBase::getMemUsage(size_t *dataShared,size_t *dataOwn,size_t *dataReal) {
	size_t pages,ownMem = 0,shMem = 0;
	size_t dReal = 0;
	{
		LockGuard<Mutex> guard(&procLock);
		for(auto p = procs.cbegin(); p != procs.cend(); ++p) {
			size_t pown = 0,psh = 0,pswap = 0;
			getMemUsageOf(p->pid,&pown,&psh,&pswap);
			ownMem += pown;
			shMem += psh;
			dReal += p->virtmem.getMemUsage(&pages);
		}
	}
	*dataOwn = ownMem * PAGE_SIZE;
	*dataShared = shMem * PAGE_SIZE;
	*dataReal = dReal + (CopyOnWrite::getFrmCount() * PAGE_SIZE);
}

int ProcBase::clone(uint8_t flags) {
	int newPid,res = 0;
	Proc *p,*cur;
	Thread *nt,*curThread = Thread::getRunning();
	assert((flags & P_ZOMBIE) == 0);
	cur = request(curThread->getProc()->pid,PLOCK_PROG);
	if(!cur) {
		res = -ESRCH;
		goto errorReqProc;
	}
	/* don't allow cloning when the process should die */
	if(cur->flags & (P_ZOMBIE | P_PREZOMBIE)) {
		res = -EINVAL;
		goto errorCur;
	}

	p = (Proc*)Cache::alloc(sizeof(Proc));
	if(!p) {
		res = -ENOMEM;
		goto errorCur;
	}

	/* set basic attributes */
	p->parentPid = cur->pid;
	memclear(p->locks,sizeof(p->locks));
	for(size_t i = 0; i < PMUTEX_COUNT; ++i)
		p->mutexes[i] = Mutex();
	p->ruid = cur->ruid;
	p->euid = cur->euid;
	p->suid = cur->suid;
	p->rgid = cur->rgid;
	p->egid = cur->egid;
	p->sgid = cur->sgid;
	p->sigRetAddr = cur->sigRetAddr;
	p->priority = cur->priority;
	p->entryPoint = cur->entryPoint;
	p->flags = flags;
	p->initProps();
	MountSpace::inherit(p,cur);

	/* give the process the same name (may be changed by exec) */
	p->command = strdup(cur->command);
	if(p->command == NULL) {
		res = -ENOMEM;
		goto errorProc;
	}

	/* determine pid; ensure that nobody can get this pid, too */
	procLock.down();
	newPid = getFreePid();
	if(newPid < 0) {
		procLock.up();
		res = -ENOPROCS;
		goto errorCmd;
	}

	/* add to processes */
	p->pid = newPid;
	add(p);
	procLock.up();

	/* create the VFS node */
	p->threadsDir = VFS::createProcess(p->pid);
	if(p->threadsDir < 0) {
		res = p->threadsDir;
		goto errorAdd;
	}

	/* clone page-dir */
	if((res = PageDir::cloneKernelspace(p->getPageDir(),curThread->getTid())) < 0)
		goto errorVFS;

	/* clone semaphores */
	if((res = Sems::clone(p,cur)) < 0)
		goto errorPdir;

	/* join group of parent */
	p->groups = NULL;
	Groups::join(p,cur);

	/* clone regions */
	p->virtmem.init(p);
	if((res = cur->virtmem.cloneAll(&p->virtmem)) < 0)
		goto errorGroups;

	/* clone current thread */
	if((res = Thread::create(curThread,&nt,p,0,true)) < 0)
		goto errorRegs;
	if(!p->threads.append(nt)) {
		res = -ENOMEM;
		goto errorThread;
	}

	/* inherit file-descriptors */
	if((res = FileDesc::clone(p)) < 0)
		goto errorThreadAppend;

	/* init arch-dependent stuff */
	if((res = cloneArch(p,cur) < 0))
		goto errorFileDescs;

	res = Thread::finishClone(curThread,nt);
	if(res == 1) {
		/* child */
		return 0;
	}
	/* parent */
	nt->unblock();

#if DEBUG_CREATIONS
	Term().writef("Thread %d (proc %d:%s): %x\n",nt->getTid(),nt->getProc()->pid,
			nt->getProc()->command,nt->getKernelStack());
#endif

	release(cur,PLOCK_PROG);
	/* if we had reserved too many, free them now */
	curThread->discardFrames();
	return p->pid;

errorFileDescs:
	FileDesc::destroy(p);
errorThreadAppend:
	p->threads.remove(nt);
errorThread:
	nt->kill();
errorRegs:
	doRemoveRegions(p,true);
errorGroups:
	Groups::leave(p->pid);
	Sems::destroyAll(p,true);
errorPdir:
	p->getPageDir()->destroy();
errorVFS:
	VFS::removeProcess(p->pid);
errorAdd:
	relRef(p);
	/* relRef does already do the next 3 operations. at least, if there are no references anymore.
	 * of course it might happen that during that time, somebody else added a reference. */
	goto errorCur;
errorCmd:
	Cache::free((void*)p->command);
errorProc:
	MountSpace::leave(p);
	Cache::free(p);
errorCur:
	release(cur,PLOCK_PROG);
errorReqProc:
	return res;
}

int ProcBase::startThread(uintptr_t entryPoint,uint8_t flags,const void *arg) {
	Thread *t = Thread::getRunning();
	Thread *nt;
	int res;
	/* don't allow new threads when the process should die */
	if(t->getProc()->flags & (P_ZOMBIE | P_PREZOMBIE))
		return -EINVAL;

	/* reserve frames for new stack- and tls-region */
	size_t pageCount = Thread::getThreadFrmCnt();
	uintptr_t tlsStart,tlsEnd;
	if(t->getTLSRange(&tlsStart,&tlsEnd))
		pageCount += BYTES_2_PAGES(tlsEnd - tlsStart);
	if(!t->reserveFrames(pageCount))
		return -ENOMEM;

	Proc *p = request(t->getProc()->pid,PLOCK_PROG);
	if(!p) {
		res = -ESRCH;
		goto errorReqProc;
	}

	if((res = Thread::create(t,&nt,p,flags,false)) < 0)
		goto error;

	/* append thread */
	if(!p->threads.append(nt)) {
		nt->kill();
		res = -ENOMEM;
		goto error;
	}

	Thread::finishThreadStart(t,nt,arg,entryPoint);

	/* mark ready (idle is always blocked because we choose it explicitly when no other can run) */
	if(nt->getFlags() & T_IDLE)
		nt->block();
	else
		nt->unblock();

#if DEBUG_CREATIONS
	Term().writef("Thread %d (proc %d:%s): %x\n",nt->getTid(),nt->getProc()->pid,
			nt->getProc()->command,nt->getKernelStack());
#endif

	release(p,PLOCK_PROG);
	t->discardFrames();
	return nt->getTid();

error:
	release(p,PLOCK_PROG);
errorReqProc:
	t->discardFrames();
	return res;
}

int ProcBase::exec(const char *path,USER const char *const *args,USER const char *const *env,
		const void *code,size_t size) {
	char *argBuffer;
	ELF::StartupInfo info;
	Thread *t = Thread::getRunning();
	Proc *p = request(t->getProc()->pid,PLOCK_PROG);
	size_t argSize = EXEC_MAX_ARGSIZE;
	int argc,envc,res,fd = -1;
	if(!p)
		return -ESRCH;
	/* don't allow exec when the process should die */
	if(p->flags & (P_ZOMBIE | P_PREZOMBIE)) {
		res = -EINVAL;
		goto error;
	}
	/* we can't do an exec if we have multiple threads (init can do that, because the threads are
	 * "kernel-threads") */
	if(p->pid != 0 && p->threads.length() > 1) {
		res = -EINVAL;
		goto error;
	}

	argc = 0;
	envc = 0;
	argBuffer = NULL;
	if(args != NULL || env != NULL) {
		/* alloc space for the arguments */
		argBuffer = (char*)Cache::alloc(EXEC_MAX_ARGSIZE);
		if(argBuffer == NULL) {
			res = -ENOMEM;
			goto error;
		}

		/* copy arguments into buffer */
		if(args != NULL) {
			argc = buildArgs(args,argBuffer,&argSize,!code);
			if(argc < 0) {
				res = argc;
				goto errorFree;
			}
		}

		/* copy env into buffer */
		if(env != NULL) {
			size_t current = EXEC_MAX_ARGSIZE - argSize;
			envc = buildArgs(env,argBuffer + current,&argSize,!code);
			if(envc < 0) {
				res = envc;
				goto errorFree;
			}
		}
	}
	argSize = EXEC_MAX_ARGSIZE - argSize;

	/* remove all except stack */
	doRemoveRegions(p,false);

	/* load program */
	if(code) {
		if(ELF::loadFromMem(code,size,&info) < 0)
			goto errorTerm;
	}
	else {
		if(ELF::loadFromFile(path,&info) < 0)
			goto errorTerm;
	}

	/* if its the dynamic linker, we need to give it the file-descriptor for the program to load */
	/* we need to do this here without lock, because VFS::openPath will perform a context-switch */
	if(info.linkerEntry != info.progEntry) {
		OpenFile *file;
		if(VFS::openPath(p->pid,VFS_READ,0,path,&file) < 0)
			goto errorTerm;
		fd = FileDesc::assoc(p,file);
		if(fd < 0) {
			file->close(p->pid);
			goto errorTerm;
		}
	}

	/* copy path so that we can identify the process */
	p->setCommand(path,argc,argBuffer);
	/* reset stats */
	p->stats.input = 0;
	p->stats.output = 0;
	p->stats.totalRuntime = 0;
	p->stats.totalSyscalls = 0;
	p->stats.totalScheds = 0;
	p->virtmem.resetStats();
	/* semaphores don't survive execs */
	Sems::destroyAll(p,false);

#if DEBUG_CREATIONS
	Term().writef("EXEC: proc %d:%s\n",p->pid,p->command);
#endif

	/* make process ready */
	/* the entry-point is the one of the process, since threads don't start with the dl again */
	p->entryPoint = info.progEntry;
	release(p,PLOCK_PROG);

	/* for starting use the linker-entry, which will be progEntry if no dl is present */
	if(!UEnv::setupProc(argc,envc,argBuffer,argSize,&info,info.linkerEntry,fd))
		goto errorTermNoRel;
	Cache::free(argBuffer);
	return 0;

errorFree:
	Cache::free(argBuffer);
error:
	release(p,PLOCK_PROG);
	return res;

errorTerm:
	release(p,PLOCK_PROG);
errorTermNoRel:
	Cache::free(argBuffer);
	terminate(1,SIG_COUNT);
	A_UNREACHED;
}

int ProcBase::join(tid_t tid,bool allowSigs) {
	Thread *t = Thread::getRunning();
	/* wait until this thread doesn't exist anymore or there are no other threads than ourself */
	Proc *p = request(t->getProc()->pid,PLOCK_PROG);
	while((tid == 0 && t->getProc()->threads.length() > 1) ||
			(tid != 0 && Thread::getById(tid) != NULL)) {
		t->wait(EV_THREAD_DIED,(evobj_t)t->getProc());
		release(p,PLOCK_PROG);

		if(allowSigs) {
			Thread::switchAway();
			if(t->hasSignal())
				return -EINTR;
		}
		else
			Thread::switchNoSigs();

		request(t->getProc()->pid,PLOCK_PROG);
	}
	release(p,PLOCK_PROG);
	return 0;
}

int ProcBase::addSignalFor(pid_t pid,int signal) {
	Proc *p = request(pid,PLOCK_PROG);
	if(!p)
		return -ENOENT;

	/* don't send a signal to processes that are dying */
	if(p->flags & (P_PREZOMBIE | P_ZOMBIE)) {
		release(p,PLOCK_PROG);
		return -ENOENT;
	}

	for(auto pt = p->threads.begin(); pt != p->threads.end(); ++pt)
		Signals::addSignalFor(*pt,signal);
	release(p,PLOCK_PROG);
	return 0;
}

void ProcBase::terminateThread(int exitCode) {
	Thread *t = Thread::getRunning();
	Proc *p = request(t->getProc()->pid,PLOCK_PROG);
	assert(p != NULL);

	/* are we the last one? */
	if(p->threads.length() == 1) {
		release(p,PLOCK_PROG);
		terminate(exitCode,SIG_COUNT);
	}
	else {
		t->terminate();
		release(p,PLOCK_PROG);

		/* we don't want to continue anymore */
		Thread::switchAway();
		A_UNREACHED;
	}
}

void ProcBase::killThread(Thread *t) {
	Proc *p = request(t->getProc()->pid,PLOCK_PROG);
	if(p) {
		t->kill();
		p->threads.remove(t);
		if(p->threads.length() == 0) {
			/* we are a zombie now, so notify parent that he can get our exit-state */
			/* this can only be done if all threads have been killed because we have to kill them
			 * before the process is killed */
			p->flags &= ~P_PREZOMBIE;
			p->flags |= P_ZOMBIE;

			/* ensure that the parent-wakeup doesn't get lost */
			LockGuard<Mutex> g(&childLock);
			notifyProcDied(p->parentPid);
		}
		release(p,PLOCK_PROG);
	}
}

void ProcBase::terminate(int exitCode,int signal) {
	Thread *t = Thread::getRunning();
	Proc *p = request(t->getProc()->getPid(),PLOCK_PROG);
	assert(p != NULL);

	/* if terminate has already been called, just terminate our own thread */
	if(p->flags & (P_PREZOMBIE | P_ZOMBIE))
		t->terminate();
	else {
		p->stats.exitCode = exitCode;
		if(signal != SIG_COUNT)
			p->stats.exitSignal = signal;

		if(p->getPid() == 0)
			Util::panic("You can't terminate the initial process");

		/* print information to log */
		if(signal != SIG_COUNT || exitCode != 0) {
			Log::get().writef("Process %d:%s terminated ",p->pid,p->command);
			if(signal != SIG_COUNT)
				Log::get().writef("by signal %d ",signal);
			Log::get().writef("with exitCode %d\n",exitCode);
		}

		/* send all other threads the kill signal */
		for(auto pt = p->threads.begin(); pt != p->threads.end(); ++pt) {
			if(*pt != t)
				Signals::addSignalFor(*pt,SIG_KILL);
		}
		p->flags |= P_PREZOMBIE;

		/* wait until all threads are dead */
		release(p,PLOCK_PROG);
		join(0,false);
		p = request(p->getPid(),PLOCK_PROG);
		assert(p != NULL);

		/* release all resources that are not necessary anymore */
		Sems::destroyAll(p,true);
		FileDesc::destroy(p);
		Groups::leave(p->pid);
		doRemoveRegions(p,true);
		MountSpace::leave(p);
		terminateArch(p);

		/* destroy our own thread, if there still is any (do that last, because this makes us
		 * a zombie) */
		assert(p->threads.length() == 1);
		t->terminate();
	}
	release(p,PLOCK_PROG);

	/* switch to somebody else */
	Thread::switchAway();
	A_UNREACHED;
}

void ProcBase::kill(pid_t pid) {
	Proc *p = request(pid,PLOCK_PROG);
	if(!p)
		return;

	/* give childs the ppid 0 */
	{
		LockGuard<Mutex> g1(&childLock);
		LockGuard<Mutex> g2(&procLock);
		for(auto cp = procs.begin(); cp != procs.end(); ++cp) {
			if(cp->parentPid == p->pid) {
				cp->parentPid = 0;
				/* if this process has already died, the parent can't wait for it since its dying
				 * right now. therefore notify init of it */
				if(cp->flags & P_ZOMBIE)
					notifyProcDied(0);
			}
		}
	}

	/* destroy pagedir and stuff (can only be done by a different process) */
	p->virtmem.destroy();
	/* now its gone, so remove it from VFS */
	VFS::removeProcess(p->pid);

	/* unref and release. if there is nobody else, we'll destroy everything */
	{
		LockGuard<SpinLock> g2(&refLock);
		p->refs--;
	}
	release(p,PLOCK_PROG);
}

void ProcBase::notifyProcDied(pid_t parent) {
	addSignalFor(parent,SIG_CHILD_TERM);
	Sched::wakeup(EV_CHILD_DIED,(evobj_t)getByPid(parent));
}

int ProcBase::waitChild(ExitState *state) {
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	/* check if there is already a dead child-proc */
	childLock.down();
	int res = getExitState(p->pid,state);
	if(res < 0) {
		/* no childs at all */
		childLock.up();
		return res;
	}

	while(res == 0) {
		/* wait for child */
		t->wait(EV_CHILD_DIED,(evobj_t)p);
		childLock.up();
		Thread::switchAway();
		/* don't continue here if we were interrupted by a signal */
		if(t->hasSignal())
			return -EINTR;
		childLock.down();
		res = getExitState(p->pid,state);
		if(res < 0)
			return res;
	}
	childLock.up();

	/* finally kill the process */
	kill(res);
	return 0;
}

int ProcBase::getExitState(pid_t ppid,ExitState *state) {
	LockGuard<Mutex> g(&procLock);
	size_t childs = 0;
	for(auto p = procs.begin(); p != procs.end(); ++p) {
		if(p->parentPid == ppid) {
			childs++;
			if((p->flags & (P_ZOMBIE | P_KILLED)) == P_ZOMBIE) {
				if(state) {
					state->pid = p->pid;
					state->exitCode = p->stats.exitCode;
					state->signal = p->stats.exitSignal;
					state->runtime = Timer::cyclesToTime(p->stats.totalRuntime);
					state->schedCount = p->stats.totalScheds;
					state->syscalls = p->stats.totalSyscalls;
					state->migrations = p->stats.totalMigrations;
					state->ownFrames = p->virtmem.getPeakOwnFrames();
					state->sharedFrames = p->virtmem.getPeakSharedFrames();
					state->swapped = p->virtmem.getSwapCount();
				}
				/* ensure that we don't kill it twice */
				p->flags |= P_KILLED;
				return p->pid;
			}
		}
	}
	return childs > 0 ? 0 : -ECHILD;
}

void ProcBase::doRemoveRegions(Proc *p,bool remStack) {
	/* unset TLS-region (and stack-region, if needed) from all threads; do this first because as
	 * soon as vmm_removeAll is finished, somebody might use the stack-region-number to get the
	 * region-range, which is not possible anymore, because the region is already gone. */
	for(auto t = p->threads.begin(); t != p->threads.end(); ++t)
		(*t)->removeRegions(remStack);
	p->virtmem.unmapAll(remStack);
}

void ProcBase::printAll(OStream &os) {
	for(auto p = procs.cbegin(); p != procs.cend(); ++p)
		p->print(os);
}

void ProcBase::printAllRegions(OStream &os) {
	for(auto p = procs.cbegin(); p != procs.cend(); ++p) {
		os.writef("Regions of proc %d (%s)\n",p->pid,p->getProgram());
		p->virtmem.printRegions(os);
		os.writef("\n");
	}
}

void ProcBase::printAllPDs(OStream &os,uint parts,bool regions) {
	for(auto p = procs.cbegin(); p != procs.cend(); ++p) {
		size_t own = 0,shared = 0,swapped = 0;
		getMemUsageOf(p->pid,&own,&shared,&swapped);
		os.writef("Process %d (%s) (%ld own, %ld sh, %ld sw):\n",
				p->pid,p->command,own,shared,swapped);
		if(regions) {
			os.writef("Regions of proc %d (%s)\n",p->pid,p->getProgram());
			p->virtmem.printRegions(os);
		}
		p->getPageDir()->print(os,parts);
		os.writef("\n");
	}
}

void ProcBase::print(OStream &os) const {
	size_t own = 0,shared = 0,swap = 0;
	os.writef("Proc %d:\n",pid);
	os.writef("\tppid=%d, cmd=%s, entry=%#Px, priority=%d, refs=%d\n",
		parentPid,command,entryPoint,priority,refs);
	os.writef("\tOwner: ruid=%u, euid=%u, suid=%u\n",ruid,euid,suid);
	os.writef("\tGroup: rgid=%u, egid=%u, sgid=%u\n",rgid,egid,sgid);
	os.writef("\tGroups: ");
	Groups::print(os,pid);
	os.writef("\n");
	getMemUsageOf(pid,&own,&shared,&swap);
	os.writef("\tFrames: own=%lu, shared=%lu, swapped=%lu\n",own,shared,swap);
	os.writef("\tExitInfo: code=%u, signal=%u\n",stats.exitCode,stats.exitSignal);
	os.writef("\tMemPeak: own=%lu, shared=%lu, swapped=%lu\n",
	           virtmem.getPeakOwnFrames() + getKMemUsage(),
	           virtmem.getPeakSharedFrames(),virtmem.getSwapCount());
	os.writef("\tRunStats: runtime=%lu, scheds=%lu, syscalls=%lu\n",
	           stats.totalRuntime,stats.totalScheds,stats.totalSyscalls);
	os.pushIndent();
	virtmem.print(os);
	FileDesc::print(os,static_cast<const Proc*>(this));
	Sems::print(os,static_cast<const Proc*>(this));
	os.popIndent();
	os.writef("\tThreads:\n");
	for(auto t = threads.cbegin(); t != threads.cend(); ++t) {
		os.writef("\t\t");
		(*t)->printShort(os);
		os.writef("\n");
	}
	os.writef("\n");
}

int ProcBase::buildArgs(USER const char *const *args,char *argBuffer,size_t *size,bool fromUser) {
	/* count args and copy them on the kernel-heap */
	/* note that we have to create a copy since we don't know where the args are. Maybe
	 * they are on the user-stack at the position we want to copy them for the
	 * new process... */
	int argc = 0;
	char *bufPos = argBuffer;
	const char *const *arg = args;
	while(1) {
		/* check if it is a valid pointer */
		if(fromUser && !PageDir::isInUserSpace((uintptr_t)arg,sizeof(char*)))
			return -EFAULT;
		/* end of list? */
		if(*arg == NULL)
			break;

		/* ensure that the argument is not longer than the left space */
		ssize_t len = UserAccess::strlen(*arg);
		if(len < 0 || len >= (ssize_t)*size)
			return -EFAULT;

		/* copy to heap */
		UserAccess::read(bufPos,*arg,len + 1);
		bufPos += len + 1;
		(*size) -= len + 1;
		arg++;
		argc++;
	}
	return argc;
}

pid_t ProcBase::getFreePid() {
	size_t count = 0;
	while(count < MAX_PROC_COUNT) {
		/* 0 is always present */
		if(nextPid >= MAX_PROC_COUNT)
			nextPid = 1;
		if(pidToProc[nextPid++] == NULL)
			return nextPid - 1;
		count++;
	}
	return INVALID_PID;
}

void ProcBase::add(Proc *p) {
	procs.append(p);
	pidToProc[p->pid] = p;
}

void ProcBase::remove(Proc *p) {
	LockGuard<Mutex> g(&procLock);
	procs.remove(p);
	pidToProc[p->pid] = NULL;
}


#if DEBUGGING

#define PROF_PROC_COUNT		128

static time_t proctimes[PROF_PROC_COUNT];

void ProcBase::startProf() {
	for(auto p = procs.cbegin(); p != procs.cend(); ++p) {
		assert(p->pid < PROF_PROC_COUNT);
		proctimes[p->pid] = 0;
		for(auto t = p->threads.cbegin(); t != p->threads.cend(); ++t)
			proctimes[p->pid] += (*t)->stats.runtime;
	}
}

void ProcBase::stopProf() {
	for(auto p = procs.cbegin(); p != procs.cend(); ++p) {
		time_t curtime = 0;
		assert(p->pid < PROF_PROC_COUNT);
		for(auto t = p->threads.cbegin(); t != p->threads.cend(); ++t)
			curtime += (*t)->stats.runtime;
		curtime -= proctimes[p->pid];
		if(curtime > 0)
			Log::get().writef("Process %3d (%18s): t=%08x\n",p->pid,p->command,curtime);
	}
}

#endif
