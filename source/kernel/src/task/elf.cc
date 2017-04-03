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

#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <log.h>
#include <string.h>
#include <util.h>
#include <video.h>

int ELF::doLoad(OpenFile *file,int type,StartupInfo *info) {
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	size_t loadSeg = 0;
	ssize_t readRes;
	int res;
	char *interpName;
	uintptr_t datPtr;

	/* first read the header */
	sElfEHeader eheader;
	if((readRes = file->read(p->getPid(),&eheader,sizeof(sElfEHeader))) != sizeof(sElfEHeader)) {
		Log::get().writef("[LOADER] Reading ELF-header of '%s' failed: %s\n",
			file->getPath(),strerror(readRes));
		goto failed;
	}

	/* check magic */
	if(memcmp(eheader.e_ident,ELFMAG,4) != 0) {
		Log::get().writef("[LOADER] Invalid magic-number '%02x%02x%02x%02x' in '%s'\n",
				eheader.e_ident[0],eheader.e_ident[1],eheader.e_ident[2],eheader.e_ident[3],
				file->getPath());
		goto failed;
	}

	/* by default set the same; the dl will overwrite it when needed */
	if(type == TYPE_PROG)
		info->linkerEntry = info->progEntry = eheader.e_entry;
	else
		info->linkerEntry = eheader.e_entry;

	/* load the LOAD segments. */
	datPtr = eheader.e_phoff;
	for(size_t j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* go to header */
		if(file->seek(p->getPid(),(off_t)datPtr,SEEK_SET) < 0) {
			Log::get().writef("[LOADER] Seeking to position 0x%Ox failed\n",(off_t)datPtr);
			goto failed;
		}
		/* read pheader */
		sElfPHeader pheader;
		if((readRes = file->read(p->getPid(),&pheader,sizeof(sElfPHeader))) != sizeof(sElfPHeader)) {
			Log::get().writef("[LOADER] Reading program-header %d of '%s' failed: %s\n",
					j,file->getPath(),strerror(readRes));
			goto failed;
		}

		if(pheader.p_type == PT_INTERP) {
			/* has to be the first segment and is not allowed for the dynamic linker */
			if(loadSeg > 0 || type != TYPE_PROG) {
				Log::get().writef("[LOADER] PT_INTERP seg is not first or we're loading the dynlinker\n");
				goto failed;
			}
			/* read name of dynamic linker */
			interpName = (char*)Cache::alloc(pheader.p_filesz);
			if(interpName == NULL) {
				Log::get().writef("[LOADER] Allocating memory for dynamic linker name failed\n");
				goto failed;
			}
			if(file->seek(p->getPid(),pheader.p_offset,SEEK_SET) < 0) {
				Log::get().writef("[LOADER] Seeking to dynlinker name (%Ox) failed\n",pheader.p_offset);
				goto failedInterpName;
			}
			if(file->read(p->getPid(),interpName,pheader.p_filesz) != (ssize_t)pheader.p_filesz) {
				Log::get().writef("[LOADER] Reading dynlinker name failed\n");
				goto failedInterpName;
			}

			/* now load him and stop loading the 'real' program */
			OpenFile *interf;
			res = VFS::openPath(p->getPid(),VFS_READ | VFS_EXEC,0,interpName,NULL,&interf);
			Cache::free(interpName);
			if(res < 0)
				return res;
			res = doLoad(interf,TYPE_INTERP,info);
			interf->close(p->getPid());
			return res;
		}

		if(pheader.p_type == PT_LOAD) {
			if(addSegment(file,&pheader,loadSeg,type,0) < 0)
				goto failed;
			loadSeg++;
		}
	}

	if(finish(file,&eheader,info) < 0)
		goto failed;
	return 0;

failedInterpName:
	Cache::free(interpName);
failed:
	return -ENOEXEC;
}

int ELF::addSegment(OpenFile *file,const sElfPHeader *pheader,size_t loadSegNo,int type,int mflags) {
	Thread *t = Thread::getRunning();
	int res,prot = 0,flags = type == TYPE_INTERP ? mflags : MAP_FIXED | mflags;
	size_t memsz = pheader->p_memsz;

	/* determine protection flags */
	if(pheader->p_flags & PF_R)
		prot |= PROT_READ;
	if(pheader->p_flags & PF_W)
		prot |= PROT_WRITE;
	if(pheader->p_flags & PF_X)
		prot |= PROT_EXEC;

	/* determine type */
	if(loadSegNo == 0) {
		/* dynamic linker has a special entrypoint */
		if(type == TYPE_INTERP && pheader->p_vaddr != INTERP_TEXT_BEGIN) {
			Log::get().writef("[LOADER] Dynamic linker text does not start at %p\n",INTERP_TEXT_BEGIN);
			return -ENOEXEC;
		}
		/* text regions are shared */
		flags |= MAP_SHARED;
	}
	else if(pheader->p_flags == (PF_R | PF_W))
		flags |= MAP_GROWABLE;
	else {
		Log::get().writef("[LOADER] Unrecognized load segment (no=%d, type=%x, flags=%x)\n",
				loadSegNo,pheader->p_type,pheader->p_flags);
		return -ENOEXEC;
	}

	/* check if the sizes are valid */
	if(pheader->p_filesz > memsz) {
		Log::get().writef("[LOADER] Number of bytes in file (%zx) more than number of bytes in mem (%zx)\n",
				pheader->p_filesz,memsz);
		return -ENOEXEC;
	}

	/* regions without binary will not be demand-loaded */
	if(file == NULL) {
		if(!t->reserveFrames(BYTES_2_PAGES(memsz))) {
			Log::get().writef("[LOADER] Unable to reserve frames for region (%zx bytes)\n",memsz);
			return -ENOMEM;
		}
	}

	/* add the region */
	VMRegion *vm;
	uintptr_t addr = pheader->p_vaddr;
	if((res = t->getProc()->getVM()->map(&addr,memsz,pheader->p_filesz,prot,flags,file,
			pheader->p_offset,&vm)) < 0) {
		Log::get().writef("[LOADER] Unable to add region: %s\n",strerror(res));
		t->discardFrames();
		return res;
	}
	t->discardFrames();
	return 0;
}
