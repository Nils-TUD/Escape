/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/task/elf.h>
#include <sys/task/proc.h>
#include <sys/task/fd.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/vmm.h>
#include <sys/mem/cache.h>
#include <sys/vfs/vfs.h>
#include <sys/log.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define ELF_TYPE_PROG		0
#define ELF_TYPE_INTERP		1

static int elf_doLoadFromFile(const char *path,uint type,sStartupInfo *info);
static int elf_addSegment(const sBinDesc *bindesc,const sElfPHeader *pheader,
		size_t loadSegNo,uint type);

int elf_loadFromFile(const char *path,sStartupInfo *info) {
	return elf_doLoadFromFile(path,ELF_TYPE_PROG,info);
}

int elf_loadFromMem(const void *code,size_t length,sStartupInfo *info) {
	size_t j,loadSegNo = 0;
	uintptr_t datPtr;
	sElfEHeader *eheader = (sElfEHeader*)code;
	sElfPHeader *pheader = NULL;

	/* check magic */
	if(memcmp(eheader->e_ident,ELFMAG,4) != 0) {
		vid_printf("[LOADER] Invalid magic-number '%02x%02x%02x%02x'\n",
				eheader->e_ident[0],eheader->e_ident[1],eheader->e_ident[2],eheader->e_ident[3]);
		return -ENOEXEC;
	}

	/* load the LOAD segments. */
	datPtr = (uintptr_t)code + eheader->e_phoff;
	for(j = 0; j < eheader->e_phnum; datPtr += eheader->e_phentsize, j++) {
		pheader = (sElfPHeader*)datPtr;
		/* check if all stuff is in the binary */
		if((uintptr_t)pheader + sizeof(sElfPHeader) >= (uintptr_t)code + length) {
			vid_printf("[LOADER] Unexpected end; pheader %d not finished\n",j);
			return -ENOEXEC;
		}

		if(pheader->p_type == PT_LOAD) {
			if(pheader->p_vaddr + pheader->p_filesz >= (uintptr_t)code + length) {
				vid_printf("[LOADER] Unexpected end; load segment %d not finished\n",loadSegNo);
				return -ENOEXEC;
			}
			if(elf_addSegment(NULL,pheader,loadSegNo,ELF_TYPE_PROG) < 0)
				return -ENOEXEC;
			/* copy the data and zero the rest, if necessary */
			paging_copyToUser((void*)pheader->p_vaddr,(void*)((uintptr_t)code + pheader->p_offset),
					pheader->p_filesz);
			paging_zeroToUser((void*)(pheader->p_vaddr + pheader->p_filesz),
					pheader->p_memsz - pheader->p_filesz);
			loadSegNo++;
		}
	}

	if(elf_finishFromMem(code,length,info) < 0)
		return -ENOEXEC;

	info->linkerEntry = info->progEntry = eheader->e_entry;
	return 0;
}

static int elf_doLoadFromFile(const char *path,uint type,sStartupInfo *info) {
	sThread *t = thread_getRunning();
	sProc *p = t->proc;
	sFile *file;
	size_t j,loadSeg = 0;
	uintptr_t datPtr;
	sElfEHeader eheader;
	sElfPHeader pheader;
	sFileInfo finfo;
	sBinDesc bindesc;
	ssize_t readRes;
	int res,fd;
	char *interpName;

	res = vfs_openPath(p->pid,VFS_READ,path,&file);
	if(res < 0) {
		vid_printf("[LOADER] Unable to open path '%s': %s\n",path,strerror(-res));
		return -ENOEXEC;
	}

	/* fill bindesc */
	if((res = vfs_fstat(p->pid,file,&finfo)) < 0) {
		vid_printf("[LOADER] Unable to stat '%s': %s\n",path,strerror(-res));
		goto failed;
	}
	bindesc.ino = finfo.inodeNo;
	bindesc.dev = finfo.device;
	bindesc.modifytime = finfo.modifytime;

	/* set suid and sgid */
	if(finfo.mode & S_ISUID)
		p->suid = finfo.uid;
	if(finfo.mode & S_ISGID)
		p->sgid = finfo.gid;

	/* first read the header */
	if((readRes = vfs_readFile(p->pid,file,&eheader,sizeof(sElfEHeader))) != sizeof(sElfEHeader)) {
		vid_printf("[LOADER] Reading ELF-header of '%s' failed: %s\n",path,strerror(-readRes));
		goto failed;
	}

	/* check magic */
	if(memcmp(eheader.e_ident,ELFMAG,4) != 0) {
		vid_printf("[LOADER] Invalid magic-number '%02x%02x%02x%02x' in '%s'\n",
				eheader.e_ident[0],eheader.e_ident[1],eheader.e_ident[2],eheader.e_ident[3],path);
		goto failed;
	}

	/* by default set the same; the dl will overwrite it when needed */
	if(type == ELF_TYPE_PROG)
		info->linkerEntry = info->progEntry = eheader.e_entry;
	else
		info->linkerEntry = eheader.e_entry;

	/* load the LOAD segments. */
	datPtr = eheader.e_phoff;
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* go to header */
		if(vfs_seek(p->pid,file,(off_t)datPtr,SEEK_SET) < 0) {
			vid_printf("[LOADER] Seeking to position 0x%Ox failed\n",(off_t)datPtr);
			goto failed;
		}
		/* read pheader */
		if((readRes = vfs_readFile(p->pid,file,&pheader,sizeof(sElfPHeader))) != sizeof(sElfPHeader)) {
			vid_printf("[LOADER] Reading program-header %d of '%s' failed: %s\n",
					j,path,strerror(-readRes));
			goto failed;
		}

		if(pheader.p_type == PT_INTERP) {
			/* has to be the first segment and is not allowed for the dynamic linker */
			if(loadSeg > 0 || type != ELF_TYPE_PROG) {
				vid_printf("[LOADER] PT_INTERP segment is not first or we're loading the dynlinker\n");
				goto failed;
			}
			/* read name of dynamic linker */
			interpName = (char*)cache_alloc(pheader.p_filesz);
			if(interpName == NULL) {
				vid_printf("[LOADER] Allocating memory for dynamic linker name failed\n");
				goto failed;
			}
			thread_addHeapAlloc(t,interpName);
			if(vfs_seek(p->pid,file,pheader.p_offset,SEEK_SET) < 0) {
				vid_printf("[LOADER] Seeking to dynlinker name (%Ox) failed\n",pheader.p_offset);
				goto failedInterpName;
			}
			if(vfs_readFile(p->pid,file,interpName,pheader.p_filesz) != (ssize_t)pheader.p_filesz) {
				vid_printf("[LOADER] Reading dynlinker name failed\n");
				goto failedInterpName;
			}
			vfs_closeFile(p->pid,file);
			/* now load him and stop loading the 'real' program */
			res = elf_doLoadFromFile(interpName,ELF_TYPE_INTERP,info);
			thread_remHeapAlloc(t,interpName);
			cache_free(interpName);
			return res;
		}

		if(pheader.p_type == PT_LOAD || pheader.p_type == PT_TLS) {
			int stype;
			stype = elf_addSegment(&bindesc,&pheader,loadSeg,type);
			if(stype < 0)
				goto failed;
			if(stype == REG_TLS) {
				uintptr_t tlsStart;
				if(thread_getTLSRange(t,&tlsStart,NULL)) {
					/* read tdata */
					if(vfs_seek(p->pid,file,(off_t)pheader.p_offset,SEEK_SET) < 0) {
						vid_printf("[LOADER] Seeking to load segment %d (%Ox) failed\n",
								loadSeg,pheader.p_offset);
						goto failed;
					}
					if((readRes = vfs_readFile(p->pid,file,(void*)tlsStart,pheader.p_filesz)) < 0) {
						vid_printf("[LOADER] Reading load segment %d failed: %s\n",
								loadSeg,strerror(-readRes));
						goto failed;
					}
					/* clear tbss */
					paging_zeroToUser((void*)(tlsStart + pheader.p_filesz),
							pheader.p_memsz - pheader.p_filesz);
				}
			}
			loadSeg++;
		}
	}

	/* introduce a file-descriptor during finishing; this way we'll close the file when segfaulting */
	if((fd = fd_assoc(t,file)) < 0)
		goto failed;
	if(elf_finishFromFile(file,&eheader,info) < 0) {
		assert(fd_unassoc(t,fd) != NULL);
		goto failed;
	}
	assert(fd_unassoc(t,fd) != NULL);
	vfs_closeFile(p->pid,file);
	return 0;

failedInterpName:
	thread_remHeapAlloc(t,interpName);
	cache_free(interpName);
failed:
	vfs_closeFile(p->pid,file);
	return -ENOEXEC;
}

static int elf_addSegment(const sBinDesc *bindesc,const sElfPHeader *pheader,
		size_t loadSegNo,uint type) {
	sThread *t = thread_getRunning();
	int stype,res;
	size_t memsz = pheader->p_memsz;
	/* determine type */
	if(loadSegNo == 0) {
		/* dynamic linker has a special entrypoint */
		if(type == ELF_TYPE_INTERP && pheader->p_vaddr != INTERP_TEXT_BEGIN) {
			vid_printf("[LOADER] Dynamic linker text does not start at %p\n",INTERP_TEXT_BEGIN);
			return -ENOEXEC;
		}
		if(pheader->p_flags != (PF_X | PF_R) ||
			(type == ELF_TYPE_PROG && pheader->p_vaddr != TEXT_BEGIN)) {
			vid_printf("[LOADER] Text does not start at %p or flags are invalid (%x)\n",
					TEXT_BEGIN,pheader->p_flags);
			return -ENOEXEC;
		}
		stype = type == ELF_TYPE_INTERP ? REG_SHLIBTEXT : REG_TEXT;
	}
	else if(pheader->p_type == PT_TLS) {
		/* not allowed for the dynamic linker */
		if(type == ELF_TYPE_INTERP) {
			vid_printf("[LOADER] TLS segment not allowed for dynamic linker\n");
			return -ENOEXEC;
		}
		stype = REG_TLS;
		/* we need the thread-control-block at the end */
		memsz += sizeof(void*);
	}
	else if(pheader->p_flags == PF_R) {
		/* not allowed for the dynamic linker */
		if(type == ELF_TYPE_INTERP) {
			vid_printf("[LOADER] Readonly-data segment not allowed for dynamic linker\n");
			return -ENOEXEC;
		}
		stype = REG_RODATA;
	}
	else if(pheader->p_flags == (PF_R | PF_W))
		stype = type == ELF_TYPE_INTERP ? REG_DLDATA : REG_DATA;
	else {
		vid_printf("[LOADER] Unrecognized load segment (no=%d, type=%x, flags=%x)\n",
				loadSegNo,pheader->p_type,pheader->p_flags);
		return -ENOEXEC;
	}

	/* check if the sizes are valid */
	if(pheader->p_filesz > memsz) {
		vid_printf("[LOADER] Number of bytes in file (%zx) more than number of bytes in mem (%zx)\n",
				pheader->p_filesz,memsz);
		return -ENOEXEC;
	}

	/* tls needs no binary */
	if(stype == REG_TLS)
		bindesc = NULL;

	/* regions without binary will not be demand-loaded */
	if(bindesc == NULL)
		thread_reserveFrames(t,BYTES_2_PAGES(memsz));

	/* add the region */
	if((res = vmm_add(t->proc->pid,bindesc,pheader->p_offset,memsz,pheader->p_filesz,stype)) < 0) {
		vid_printf("[LOADER] Unable to add region: %s\n",strerror(-res));
		thread_discardFrames(t);
		return res;
	}
	if(stype == REG_TLS)
		thread_setTLSRegion(t,res);
	thread_discardFrames(t);
	return stype;
}
