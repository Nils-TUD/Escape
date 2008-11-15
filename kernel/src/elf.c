/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/paging.h"
#include "../h/elf.h"
#include "../h/util.h"
#include "../h/video.h"

u32 elf_loadprog(u8 *code) {
	u32 seenLoadSegments = 0;
	tProc *p = proc_getRunning();

	u32 j;
	u8 const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	if(*(u32*)eheader->e_ident != *(u32*)ELFMAG) {
		vid_printf("Error: Invalid magic-number\n");
		return ELF_INVALID_ENTRYPOINT;
	}

	p->textPages = 0;
	p->dataPages = 0;

	/* load the LOAD segments. */
	for(datPtr = (u8 const*)(code + eheader->e_phoff), j = 0; j < eheader->e_phnum;
		datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		if(pheader->p_type == PT_LOAD) {
			u32 pages;
			u8 const* segmentSrc;
			segmentSrc = code + pheader->p_offset;

			/* get to know the lowest virtual address. must be 0x0.  */
			if(seenLoadSegments == 0) {
				if(pheader->p_vaddr != 0) {
					vid_printf("Error: p_vaddr != 0\n");
					return ELF_INVALID_ENTRYPOINT;
				}
			}
			else if(seenLoadSegments == 2) {
				/* uh oh a third LOAD segment. that's not allowed
				* indeed */
				vid_printf("Error: too many LOAD segments\n");
				return ELF_INVALID_ENTRYPOINT;
			}

			/* Note that we put everything in the data-segment here because otherwise we would
			* steal the text from the parent-process after fork, exec & exit */
			pages = BYTES_2_PAGES(pheader->p_memsz);
			if(seenLoadSegments != 0) {
				if(pheader->p_vaddr & (PAGE_SIZE - 1))
					pages++;
			}

			/* get more space for the data area. */
			proc_changeSize(pages,CHG_DATA);

			/* copy the data, and zero remaining bytes */
			memcpy((void*)pheader->p_vaddr, (void*)segmentSrc, pheader->p_filesz);
			memset((void*)(pheader->p_vaddr + pheader->p_filesz), 0, pheader->p_memsz - pheader->p_filesz);
			++seenLoadSegments;
		}
	}

	return (u32)eheader->e_entry;
}
