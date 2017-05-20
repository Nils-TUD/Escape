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

#include <dbg/kb.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <task/thread.h>
#include <usergroup/usergroup.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <boot.h>
#include <common.h>
#include <config.h>
#include <log.h>
#include <util.h>
#include <video.h>

static const size_t MAX_ARG_COUNT	= 16;
static const size_t MAX_ARG_LEN		= 256;

static const int KERNEL_PERCENT		= 40;

static const size_t BAR_HEIGHT		= 1;
static const size_t BAR_WIDTH		= 60;
static const size_t BAR_PAD			= 10;
static const size_t BAR_TEXT_PAD	= 1;

extern void (*CTORS_BEGIN)();
extern void (*CTORS_END)();

Boot::Info Boot::info;
void (*Boot::unittests)() = NULL;

static size_t finished = 0;
extern void *_btext;
extern void *_ebss;

static inline size_t barPadX() {
	return (VID_COLS - BAR_WIDTH) / 2;
}
static inline size_t barPadY() {
	return (VID_ROWS / 2) - ((BAR_HEIGHT + 2) / 2) - 1;
}

void Boot::start(void *info) {
	for(void (**func)() = &CTORS_BEGIN; func != &CTORS_END; func++)
		(*func)();

	/* necessary during initialization until we have a thread */
	Thread::setRunning(NULL);

	archStart(info);
	drawProgressBar();
	for(size_t i = 0; i < taskList.count; i++) {
		const BootTask *task = taskList.tasks + i;
		Log::get().writef("%s\n",task->name);
		taskStarted(task->name);
		task->execute();
		taskFinished();
	}

	Log::get().writef("%d free frames (%d KiB)\n",PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF),
			PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF) * PAGE_SIZE / 1024);

	if(DISABLE_DEMLOAD)
		Log::get().writef("Warning: demand loading is disabled!\n");

	/* initloader is going to load the boot modules now */
	taskStarted("Loading boot modules...");
}

void Boot::taskStarted(const char *text) {
	if(!Config::get(Config::LOG_TO_VGA)) {
		Video::get().goTo(barPadY() + BAR_HEIGHT + 2 + BAR_TEXT_PAD,barPadX());
		Video::get().writef("%-*s",VID_COLS - barPadX() * 2,text);
	}
}

void Boot::taskFinished() {
	if(!Config::get(Config::LOG_TO_VGA)) {
		const uint width = BAR_WIDTH - 3;
		finished++;
		/* our tasks + the boot modules */
		size_t total = taskList.count + 1;
		uint percent = (KERNEL_PERCENT * finished) / total;
		uint filled = (width * percent) / 100;
		Video::get().goTo(barPadY() + 1,barPadX() + 1);
		if(filled)
			Video::get().writef("\033[co;0;7]%*s\033[co]",filled," ");
		if(width - filled)
			Video::get().writef("%*s",width - filled," ");
	}
}

static char *filename(const char *path) {
	const char *name = path;
	const char *slash = name;
	while(*name) {
		if(*name == ' ')
			break;
		if(*name == '/')
			slash = name + 1;
		name++;
	}

	size_t len = name - slash;
	char *res = (char*)Cache::alloc(len + 1);
	if(!res)
		Util::panic("Not enough memory");
	memcpy(res,slash,len);
	res[len] = '\0';
	return res;
}

void Boot::createModFiles() {
	const fs::User kern = fs::User::kernel();

	VFSNode *node = NULL;
	int res = VFSNode::request(kern,"/sys/boot",&node,VFS_WRITE,0);
	if(res < 0)
		Util::panic("Unable to resolve /sys/boot");

	VFSNode *args = createObj<VFSFile>(kern,node,strdup("arguments"),S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
	if(!args)
		Util::panic("Unable to create /sys/boot/arguments");

	OpenFile *file;
	if(VFS::openFile(kern,0,VFS_WRITE,args,args->getNo(),VFS_DEV_NO,&file) < 0)
		Util::panic("Unable to open /sys/boot/arguments");

	for(auto mod = modsBegin(); mod != modsEnd(); ++mod) {
		char *modname = filename(mod->name);
		VFSNode *n = createObj<VFSFile>(kern,node,modname,(void*)mod->virt,mod->size,FILE_DEF_MODE);
		if(!n)
			Util::panic("Unable to create mbmod-file for '%s'",modname);
		if(n->chown(kern,ROOT_UID,GROUP_DRIVER) != 0)
			Util::panic("Unable to chown mbmod-file for '%s'",modname);
		if(n->chmod(kern,S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH) != 0)
			Util::panic("Unable to chmod mbmod-file for '%s'",modname);
		VFSNode::release(n);

		if(file->write(KERNEL_PID,mod->name,strlen(mod->name)) < 0)
			Util::panic("Unable to write arguments to /sys/boot/arguments");
		if(file->write(KERNEL_PID,"\n",1) < 0)
			Util::panic("Unable to write arguments to /sys/boot/arguments");
	}

	VFSNode::release(args);
	VFSNode::release(node);
	file->close();
}

void Boot::parseCmdline() {
	int argc;
	const char **argv = Boot::parseArgs((char*)info.cmdLine,&argc);
	Log::get().writef("Kernel parameters: ");
	for(int i = 0; i < argc; ++i)
		Log::get().writef("%s ",argv[i]);
	Log::get().writef("\n");
	Config::parseBootParams(argc,argv);
}

size_t Boot::getKernelSize() {
	uintptr_t start = (uintptr_t)&_btext;
	uintptr_t end = (uintptr_t)&_ebss;
	return end - start;
}

size_t Boot::getModuleSize() {
	if(info.modCount == 0)
		return 0;
	uintptr_t start = info.mods[0].virt;
	uintptr_t end = info.mods[info.modCount - 1].virt + info.mods[info.modCount - 1].size;
	return end - start;
}

uintptr_t Boot::getModuleRange(const char *name,size_t *size) {
	for(auto mod = modsBegin(); mod != modsEnd(); ++mod) {
		if(strcmp(mod->name,name) == 0) {
			*size = mod->size;
			return mod->phys;
		}
	}
	return 0;
}

void Boot::print(OStream &os) {
	os.writef("cmdLine: %s\n",info.cmdLine);

	os.writef("modCount: %zu\n",info.modCount);
	for(size_t i = 0; i < info.modCount; i++) {
		os.writef("\t[%zu]: virt: %p..%p\n",i,info.mods[i].virt,info.mods[i].virt + info.mods[i].size);
		os.writef("\t     phys: %p..%p\n",info.mods[i].phys,info.mods[i].phys + info.mods[i].size);
		os.writef("\t     cmdline: %s\n",info.mods[i].name ? info.mods[i].name : "<NULL>");
	}

	os.writef("mmapCount: %zu\n",info.mmapCount);
	for(size_t i = 0; i < info.mmapCount; i++) {
		os.writef("\t%zu: addr=%#012Lx, size=%#012zx, type=%s\n",
				i,info.mmap[i].baseAddr,info.mmap[i].size,
				info.mmap[i].type == 1 ? "free" : "used");
	}
}

const char **Boot::parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
				if(j + 2 >= MAX_ARG_COUNT)
					break;
				args[j][i] = '\0';
				j++;
				i = 0;
				args[j] = argvals[j];
			}
		}
		else if(i < MAX_ARG_LEN)
			args[j][i++] = *line;
		line++;
	}
	*argc = j + 1;
	args[j][i] = '\0';
	args[j + 1] = NULL;
	return (const char**)args;
}

void Boot::drawProgressBar() {
	if(!Config::get(Config::LOG_TO_VGA)) {
		Video &vid = Video::get();
		vid.clearScreen();
		/* top */
		vid.goTo(barPadY(),barPadX());
		vid.writef("\xC9");
		for(ushort x = 1; x < BAR_WIDTH - 2; x++)
			vid.writef("\xCD");
		vid.writef("\xBB");
		/* left and right */
		for(ushort y = 0; y < BAR_HEIGHT; y++) {
			vid.goTo(barPadY() + 1 + y,barPadX());
			vid.writef("\xBA");
			vid.goTo(barPadY() + 1 + y,VID_COLS - (barPadX() + 2));
			vid.writef("\xBA");
		}
		/* bottom */
		vid.goTo(barPadY() + BAR_HEIGHT + 1,barPadX());
		vid.writef("\xC8");
		for(ushort x = 1; x < VID_COLS - (barPadX() * 2 + 2); x++)
			vid.writef("\xCD");
		vid.writef("\xBC");
	}
}
