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

#pragma once

#include <sys/common.h>
#include <sys/mem/region.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/vmtree.h>
#include <sys/mem/vmfreemap.h>
#include <sys/task/elf.h>
#include <sys/task/env.h>
#include <sys/task/thread.h>
#include <sys/task/groups.h>
#include <sys/task/sems.h>
#include <sys/task/mountspace.h>
#include <sys/vfs/fs.h>
#include <sys/col/slist.h>
#include <sys/col/islist.h>
#include <sys/spinlock.h>
#include <sys/mutex.h>
#include <sys/interrupts.h>
#include <assert.h>

/* max number of coexistent processes */
#define MAX_PROC_COUNT		8192
#define MAX_FD_COUNT		1024
#define MAX_SEM_COUNT		256

/* for marking unused */
#define INVALID_PID			(MAX_PROC_COUNT + 1)
#define KERNEL_PID			MAX_PROC_COUNT

#define ROOT_UID			0
#define ROOT_GID			0

/* process flags */
#define P_ZOMBIE			1
#define P_PREZOMBIE			2
#define P_BOOT				4
#define P_KILLED			8

#define PLOCK_COUNT			3
#define PMUTEX_COUNT		1
#define PLOCK_FDS			0
#define PLOCK_SEMS			1
#define PLOCK_PORTS			2
#define PLOCK_PROG			3	/* clone, exec, threads and virtmem */

class Groups;
class FileDesc;
class VFSFS;
class Env;

/* represents a process */
class ProcBase : public SListItem {
	friend class Groups;
	friend class FileDesc;
	friend class VFSFS;
	friend class Env;
	friend class Sems;
	friend class ThreadBase;
	friend class MountSpace;

protected:
	ProcBase() {
	}

public:
	struct ExitState {
		pid_t pid;
		/* the signal that killed the process (SIG_COUNT if none) */
		int signal;
		/* exit-code the process gave us via exit() */
		int exitCode;
		/* memory it has used */
		ulong ownFrames;
		ulong sharedFrames;
		ulong swapped;
		/* other stats */
		uint64_t runtime;
		ulong schedCount;
		ulong syscalls;
		ulong migrations;
	};

	struct Stats {
		/* thread stats */
		uint64_t totalRuntime;
		uint64_t lastCycles;
		ulong totalSyscalls;
		ulong totalScheds;
		ulong totalMigrations;
		/* I/O stats */
		ulong input;
		ulong output;
		/* exit */
		ushort exitCode;
		ushort exitSignal;
	};

	/**
	 * Inits the page-directory
	 */
	static void preinit();

	/**
	 * Initializes the process-management
	 */
	static void init();

	/**
	 * @return the page-dir of the current process (or the first one)
	 */
	static PageDir *getCurPageDir();

	/**
	 * @return the pid of the running process
	 */
	static pid_t getRunning();

	/**
	 * Returns the process with given id WITHOUT aquiring a lock.
	 *
	 * @param pid the pid of the process
	 * @return the process with given pid
	 */
	static Proc *getByPid(pid_t pid);

	/**
	 * Checks whether the given process exists and adds a reference if so.
	 *
	 * @param pid the pid of the process
	 * @return if it exists, the process
	 */
	static Proc *getRef(pid_t pid);

	/**
	 * Releases the reference of given process that has been added previously by getRef(). If there
	 * are no further references, the process is destroyed.
	 *
	 * @param p the process
	 */
	static void relRef(const Proc *p);

	/**
	 * Requests the process with given id and given lock.
	 *
	 * @param pid the process-id
	 * @param l the lock: PLOCK_*
	 * @return the process or NULL
	 */
	static Proc *request(pid_t pid,size_t l);

	/**
	 * Tries to request the process lock. Does only work for the mutex-locks.
	 *
	 * @param pid the process-id
	 * @param l the lock: PLOCK_*
	 * @return the process or NULL
	 */
	static Proc *tryRequest(pid_t pid,size_t l);

	/**
	 * Releases the lock of given process.
	 *
	 * @param p the process
	 * @param l the lock: PLOCK_*
	 */
	static void release(const Proc *p,size_t l);

	/**
	 * @return the number of existing processes
	 */
	static size_t getCount();

	/**
	 * Determines the mem-usage of all processes
	 *
	 * @param dataShared will point to the number of shared bytes
	 * @param dataOwn will point to the number of own bytes
	 * @param dataReal will point to the number of really used bytes (considers sharing)
	 */
	static void getMemUsage(size_t *dataShared,size_t *dataOwn,size_t *dataReal);

	/**
	 * Determines the memory-usage for the given process
	 *
	 * @param pid the process-id
	 * @param own will be set to the number of own frames
	 * @param shared will be set to the number of shared frames
	 * @param swapped will be set to the number of swapped frames
	 */
	static void getMemUsageOf(pid_t pid,size_t *own,size_t *shared,size_t *swapped);

	/**
	 * Adds the given signal to the given process. If necessary, the process is killed.
	 *
	 * @param pid the process-id
	 * @param signal the signal
	 * @return 0 on success
	 */
	static int addSignalFor(pid_t pid,int signal);

	/**
	 * Copies the arguments (for exec) in <args> to <*argBuffer> and takes care that everything
	 * is ok. <*argBuffer> will be allocated on the heap, so that is has to be free'd when you're done.
	 *
	 * @param args the arguments to copy
	 * @param argBuffer will point to the location where it has been copied (to be used by
	 * 	Proc::setupUserStack())
	 * @param size will point to the size the arguments take in <argBuffer>
	 * @param fromUser whether the arguments are from user-space
	 * @return the number of arguments on success or < 0
	 */
	static int buildArgs(const char *const *args,char **argBuffer,size_t *size,bool fromUser);

	/**
	 * Clones the current process, gives the new process a clone of the current thread and saves this
	 * thread in Proc::clone() so that it will start there on thread_resume().
	 *
	 * @param flags the flags to set for the process (e.g. P_VM86)
	 * @return < 0 if an error occurred, the child-pid for parent, 0 for child
	 */
	static int clone(uint8_t flags);

	/**
	 * Starts a new thread at given entry-point. Will clone the kernel-stack from the current thread
	 *
	 * @param entryPoint the address where to start
	 * @param flags the thread-flags
	 * @param arg the argument
	 * @return < 0 if an error occurred or the new tid
	 */
	static int startThread(uintptr_t entryPoint,uint8_t flags,const void *arg);

	/**
	 * Executes the given program, i.e. removes all regions except the stacks and loads the new one.
	 *
	 * @param path the path to the program
	 * @param args the arguments
	 * @param code if the program is already in memory, the location of it (used for boot-modules)
	 * @param size if code != NULL, the size of the program in memory
	 * @return 0 on success
	 */
	static int exec(const char *path,const char *const *args,const void *code,size_t size);

	/**
	 * Waits until the thread with given thread-id or all other threads of the process are terminated.
	 *
	 * @param tid the thread to wait for (0 = all other threads)
	 */
	static void join(tid_t tid);

	/**
	 * Waits until a child-process terminated and copies its exit-state to <state>.
	 *
	 * @param state the state to write to (if not NULL)
	 * @return 0 on success
	 */
	static int waitChild(ExitState *state);

	/**
	 * Removes all regions from the given process
	 *
	 * @param pid the process-id
	 * @param remStack whether the stack should be removed too
	 */
	static void removeRegions(pid_t pid,bool remStack);

	/**
	 * Terminates the current thread (volunatily). If it's the only thread in the process, the
	 * complete process will terminated.
	 *
	 * @param exitCode the exit-code
	 */
	A_NORETURN static void terminateThread(int exitCode) asm("proc_exit");

	/**
	 * Kills the given thread, i.e. releases the last resources and removes it from the process
	 *
	 * @param t the thread
	 */
	static void killThread(Thread *t);

	/**
	 * Marks the current process as zombie and notifies the waiting parent thread. As soon as the
	 * parent thread fetches the exit-state we'll kill the process.
	 *
	 * @param pid the process-id
	 * @param exitCode the exit-code to store
	 * @param signal the signal with which it was killed (SIG_COUNT if none)
	 */
	A_NORETURN static void terminate(int exitCode,int signal = SIG_COUNT);

	/**
	 * Kills the given process. This is done after the exit-state has been given to the parent.
	 *
	 * @param pid the process-id
	 */
	static void kill(pid_t pid);

	/**
	 * Prints all existing processes
	 *
	 * @param os the output-stream
	 */
	static void printAll(OStream &os);

	/**
	 * Prints the regions of all existing processes
	 *
	 * @param os the output-stream
	 */
	static void printAllRegions(OStream &os);

	/**
	 * Prints the given parts of the page-directory for all existing processes
	 *
	 * @param os the output-stream
	 * @param parts the parts (see paging_printPageDirOf)
	 * @param regions whether to print the regions too
	 */
	static void printAllPDs(OStream &os,uint parts,bool regions);

#if DEBUGGING
	/**
	 * Starts profiling all processes
	 */
	static void startProf();

	/**
	 * Stops profiling all processes and prints the result
	 */
	static void stopProf();
#endif

	/**
	 * Requests the given lock of this process
	 *
	 * @param l the lock
	 */
	void lock(size_t l) const;
	/**
	 * Tries to request the given lock of this process. This has to be a mutex!
	 *
	 * @param l the lock
	 */
	bool tryLock(size_t l) const;
	/**
	 * Releases the given lock of this process
	 *
	 * @param l the lock
	 */
	void unlock(size_t l) const;

	/**
	 * @return the process id
	 */
	pid_t getPid() const {
		return pid;
	}
	/**
	 * @return the parent process id
	 */
	pid_t getParentPid() const {
		return parentPid;
	}
	/**
	 * @return the flags (P_*)
	 */
	uint8_t getFlags() const {
		return flags;
	}
	/**
	 * @return the minimum priority of all threads in this process
	 */
	uint8_t getPriority() const {
		return priority;
	}
	/**
	 * @return real, effective or saved user-id
	 */
	uid_t getRUid() const {
		return ruid;
	}
	uid_t getEUid() const {
		return euid;
	}
	uid_t getSUid() const {
		return suid;
	}
	void setUid(uid_t uid) {
		ruid = euid = suid = uid;
	}
	void setEUid(uid_t uid) {
		euid = uid;
	}
	void setSUid(uid_t uid) {
		suid = uid;
	}
	/**
	 * @return real, effective or saved group-id
	 */
	gid_t getRGid() const {
		return rgid;
	}
	gid_t getEGid() const {
		return egid;
	}
	gid_t getSGid() const {
		return sgid;
	}
	void setGid(gid_t gid) {
		rgid = egid = sgid = gid;
	}
	void setEGid(gid_t gid) {
		egid = gid;
	}
	void setSGid(gid_t gid) {
		sgid = gid;
	}
	/**
	 * @return the command line of the process
	 */
	const char *getCommand() const {
		return command;
	}
	/**
	 * @return the program name (statically allocated)
	 */
	const char *getProgram() const;
	/**
	 * @return the virtual memory object for this process
	 */
	VirtMem *getVM() {
		return &virtmem;
	}
	/**
	 * @return the page-directory
	 */
	PageDir *getPageDir() {
		return virtmem.getPageDir();
	}
	const PageDir *getPageDir() const {
		return virtmem.getPageDir();
	}
	/**
	 * @return the entry-point of the program
	 */
	uintptr_t getEntryPoint() const {
		return entryPoint;
	}
	/**
	 * The address of the signal-return-handler
	 */
	uintptr_t getSigRetAddr() const {
		return sigRetAddr;
	}
	void setSigRetAddr(uintptr_t addr) {
		sigRetAddr = addr;
	}
	/**
	 * @return the number of threads
	 */
	size_t getThreadCount() const {
		return threads.length();
	}

	/**
	 * @return the statistics for this process
	 */
	Stats &getStats() {
		return stats;
	}
	const Stats &getStats() const {
		return stats;
	}

	/**
	 * @return the total runtime of this process
	 */
	uint64_t getRuntime() const {
		return Timer::cyclesToTime(stats.totalRuntime);
	}

	/**
	 * @return the threads-directory in the VFS
	 */
	inode_t getThreadsDir() const {
		return threadsDir;
	}

	/**
	 * @return the main thread of this process
	 */
	Thread *getMainThread() {
		return *threads.begin();
	}

	/**
	 * Adds/subtracts own-, shared- or swapped frames to/from the process.
	 *
	 * @param amount the number of frames (positive or negative)
	 */
	void addOwn(long amount);
	void addShared(long amount);
	void addSwap(long amount);

	/**
	 * Sets the command to <cmd> and frees the current command-string, if necessary
	 *
	 * @param cmd the new command-name
	 * @param argc the number of arguments
	 * @param args the arguments, one after another, separated by '\0'
	 */
	void setCommand(const char *cmd,int argc,const char *args);

	/**
	 * @return the number of frames that is used by the process in the kernel-space
	 */
	size_t getKMemUsage() const;

	/**
	 * Prints this process
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;

private:
	/**
	 * Initializes the architecture specific parts of the given process
	 *
	 * @param dst the clone
	 * @param src the parent
	 * @return 0 on success
	 */
	static int cloneArch(Proc *dst,const Proc *src);

	/**
	 * Handles the architecture-specific part of the terminate-operation.
	 *
	 * @param p the process
	 */
	static void terminateArch(Proc *p);

	void initProps();
	static void notifyProcDied(pid_t parent);
	static int getExitState(pid_t ppid,ExitState *state);
	static void doRemoveRegions(Proc *p,bool remStack);
	static pid_t getFreePid();
	static void add(Proc *p);
	static void remove(Proc *p);

	/* flags for vm86 and zombie */
	uint8_t flags;
	/* process id (2^16 processes should be enough :)) */
	/*const*/ pid_t pid;
	/* parent process id */
	pid_t parentPid;
	/* real, effective and saved user-id */
	uid_t ruid;
	uid_t euid;
	uid_t suid;
	/* real, effective and saved group-id */
	gid_t rgid;
	gid_t egid;
	gid_t sgid;
	/* the minimum priority of all threads; is used for new childs and threads */
	uint8_t priority;
	/* the number of references to this process */
	mutable ushort refs;
	/* the entrypoint of the binary */
	uintptr_t entryPoint;
	VirtMem virtmem;
	/* all groups (may include egid or not) of this process */
	Groups::Entries *groups;
	/* file descriptors: point into the global file table */
	OpenFile **fileDescs;
	size_t fileDescsSize;
	/* process local semaphores */
	Sems::Entry **sems;
	size_t semsSize;
	/* the mount space */
	MountSpace *mounts;
	/* environment-variables of this process */
	SList<Env::EnvVar> *env;
	/* the directory-node-number in the VFS of this process */
	inode_t threadsDir;
	Stats stats;
	/* the address of the sigRet "function" */
	uintptr_t sigRetAddr;
	/* start-command */
	const char *command;
	/* threads of this process */
	ISList<Thread*> threads;
	/* locks for this process */
	mutable SpinLock locks[PLOCK_COUNT];
	mutable Mutex mutexes[PMUTEX_COUNT];

	static Proc first;
	static SList<Proc> procs;
	static Proc *pidToProc[MAX_PROC_COUNT];
	static pid_t nextPid;
	static Mutex procLock;
	static Mutex childLock;
	static SpinLock refLock;
};

#ifdef __i386__
#include <sys/arch/i586/task/proc.h>
static_assert(sizeof(Proc) <= 512,"Proc is too big");
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/proc.h>
static_assert(sizeof(Proc) <= 512,"Proc is too big");
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/proc.h>
static_assert(sizeof(Proc) <= 1024,"Proc is too big");
#endif

/* the area for ProcBase::chgsize() */
typedef enum {CHG_DATA,CHG_STACK} eChgArea;

inline void ProcBase::preinit() {
	first.getPageDir()->makeFirst();
}

inline PageDir *ProcBase::getCurPageDir() {
	const Thread *t = Thread::getRunning();
	/* just needed at the beginning */
	if(t == NULL)
		return first.getPageDir();
	return t->getProc()->getPageDir();
}

inline pid_t ProcBase::getRunning() {
	const Thread *t = Thread::getRunning();
	return t->getProc()->getPid();
}

inline Proc *ProcBase::getByPid(pid_t pid) {
	if(pid >= ARRAY_SIZE(pidToProc))
		return NULL;
	return pidToProc[pid];
}

inline Proc *ProcBase::request(pid_t pid,size_t l) {
	Proc *p = getRef(pid);
	if(p)
		p->lock(l);
	return p;
}

inline Proc *ProcBase::tryRequest(pid_t pid,size_t l) {
	Proc *p = getRef(pid);
	if(p) {
		if(!p->tryLock(l)) {
			relRef(p);
			return NULL;
		}
	}
	return p;
}

inline void ProcBase::release(const Proc *p,size_t l) {
	p->unlock(l);
	relRef(p);
}

inline void ProcBase::lock(size_t l) const {
	if(l == PLOCK_PROG)
		mutexes[l - PLOCK_COUNT].down();
	else
		locks[l].down();
}

inline bool ProcBase::tryLock(size_t l) const {
	assert(l == PLOCK_PROG);
	return mutexes[l - PLOCK_COUNT].tryDown();
}

inline void ProcBase::unlock(size_t l) const {
	if(l == PLOCK_PROG)
		mutexes[l - PLOCK_COUNT].up();
	else
		locks[l].up();
}

inline size_t ProcBase::getCount() {
	return procs.length();
}

inline void ProcBase::removeRegions(pid_t pid,bool remStack) {
	Proc *p = request(pid,PLOCK_PROG);
	doRemoveRegions(p,remStack);
	release(p,PLOCK_PROG);
}
