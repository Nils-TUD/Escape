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

#include <task/proc.h>
#include <vfs/file.h>
#include <common.h>

class VFSInfo {
	/* callback function for the default read-handler */
	typedef void (*read_func)(VFSNode *node,size_t *dataSize,void **buffer);

#define GEN_INFO_FILECLASS(className,fileName,callback)												\
	class className : public VFSFile {																\
	public:																							\
		explicit className(const fs::User &u,VFSNode *parent,char *name,uint mode,bool &success)	\
			: VFSFile(u,parent,name,NULL,0,mode,success) {											\
		}																							\
		explicit className(const fs::User &u,VFSNode *parent,bool &success)							\
			: className(u,parent,(char*)(fileName),0444,success) {									\
		}																							\
		virtual ssize_t read(pid_t,OpenFile *,void *buffer,off_t offset,size_t count) override {	\
			ssize_t res = VFSInfo::readHelper(this,buffer,offset,count,0,(callback));				\
			acctime = Timer::getTime();																\
			return res;																				\
		}																							\
		virtual ssize_t write(pid_t,OpenFile *,const void *,off_t,size_t) override {				\
			return -ENOTSUP;																		\
		}																							\
		virtual int truncate(off_t) override {														\
			return -ENOTSUP;																		\
		}																							\
	};

	static void traceReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void procReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void threadReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void regionsReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void mapsReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void virtMemReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void cpuReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void statsReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void memUsageReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void selfLinkReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void pidLinkReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void mountsReadCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void msLinkReadCallback(VFSNode *node,size_t *dataSize,void **buffer);

public:
	/**
	 * Creates the vfs-info nodes
	 */
	static void init(VFSNode *sysNode);

	GEN_INFO_FILECLASS(TraceFile,"trace",traceReadCallback);
	GEN_INFO_FILECLASS(ProcFile,"info",procReadCallback);
	GEN_INFO_FILECLASS(ThreadFile,"info",threadReadCallback);
	GEN_INFO_FILECLASS(RegionsFile,"regions",regionsReadCallback);
	GEN_INFO_FILECLASS(MapFile,"map",mapsReadCallback);
	GEN_INFO_FILECLASS(VirtMemFile,"virtmem",virtMemReadCallback);
	GEN_INFO_FILECLASS(CPUFile,"cpu",cpuReadCallback);
	GEN_INFO_FILECLASS(StatsFile,"stats",statsReadCallback);
	GEN_INFO_FILECLASS(MemUsageFile,"memusage",memUsageReadCallback);
	GEN_INFO_FILECLASS(SelfLinkFile,"",selfLinkReadCallback);
	GEN_INFO_FILECLASS(PidLinkFile,"",pidLinkReadCallback);
	GEN_INFO_FILECLASS(MountsFile,"info",mountsReadCallback);
	GEN_INFO_FILECLASS(MSLinkFile,"ms",msLinkReadCallback);

	static ssize_t readHelper(VFSNode *node,void *buffer,off_t offset,size_t count,
		size_t dataSize,read_func callback);

private:
	static Proc *getProc(VFSNode *node,size_t *dataSize,void **buffer);
	static Thread *getThread(VFSNode *node,size_t *dataSize,void **buffer);
};
