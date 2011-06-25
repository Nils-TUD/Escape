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
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/elf.h>
#include <stdlib.h>
#include <stdio.h>

#if ELF_TYPE == 64
#	define POFFW	"16"
#	define PADDRW	"16"
#else
#	define POFFW	"8"
#	define PADDRW	"8"
#endif

#define MAX_SYM_LEN		40

static const char *fileTypes[] = {
	/* ET_NONE */			"NONE (No file type)",
	/* ET_REL */			"REL (Relocatable file)",
	/* ET_EXEC */			"EXEC (Executable file)",
	/* ET_DYN */			"DYN (Shared object file)",
	/* ET_CORE */			"CORE (Core file)",
};

static const char *secTypes[] = {
	/* SHT_NULL */			"NULL",
	/* SHT_PROGBITS */		"PROGBITS",
	/* SHT_SYMTAB */		"SYMTAB",
	/* SHT_STRTAB */		"STRTAB",
	/* SHT_RELA */			"RELA",
	/* SHT_HASH */			"HASH",
	/* SHT_DYNAMIC */		"DYNAMIC",
	/* SHT_NOTE */			"NOTE",
	/* SHT_NOBITS */		"NOBITS",
	/* SHT_REL */			"REL",
	/* SHT_SHLIB */			"SHLIB",
	/* SHT_DYNSYM */		"DYNSYM",
	/* SHT_INIT_ARRAY */	"INIT_ARRAY",
	/* SHT_FINI_ARRAY */	"FINI_ARRAY",
	/* SHT_PREINIT_ARRAY */	"PREINIT_ARRAY",
	/* SHT_GROUP */			"GROUP",
	/* SHT_SYMTAB_SHNDX */	"SYMTAB_SHNDX",
};

static const char *progTypes[] = {
	/* PT_NULL */			"NULL",
	/* PT_LOAD */			"LOAD",
	/* PT_DYNAMIC */		"DYNAMIC",
	/* PT_INTERP */			"INTERP",
	/* PT_NOTE */			"NOTE",
	/* PT_SHLIB */			"SHLIB",
	/* PT_PHDR */			"PHDR",
	/* PT_TLS */			"TLS",
};

static const char *relocTypes[] = {
	/* DT_NULL */			"(NULL)",
	/* DT_NEEDED */			"(NEEDED)",
	/* DT_PLTRELSZ */		"(PLTRELSZ)",
	/* DT_PLTGOT */			"(PLTGOT)",
	/* DT_HASH */			"(HASH)",
	/* DT_STRTAB */			"(STRTAB)",
	/* DT_SYMTAB */			"(SYMTAB)",
	/* DT_RELA */			"(RELA)",
	/* DT_RELASZ */			"(RELASZ)",
	/* DT_RELAENT */		"(RELAENT)",
	/* DT_STRSZ */			"(STRSZ)",
	/* DT_SYMENT */			"(SYMENT)",
	/* DT_INIT */			"(INIT)",
	/* DT_FINI */			"(FINI)",
	/* DT_SONAME */			"(SONAME)",
	/* DT_RPATH */			"(RPATH)",
	/* DT_SYMBOLIC */		"(SYMBOLIC)",
	/* DT_REL */			"(REL)",
	/* DT_RELSZ */			"(RELSZ)",
	/* DT_RELENT */			"(RELENT)",
	/* DT_PLTREL */			"(PLTREL)",
	/* DT_DEBUG */			"(DEBUG)",
	/* DT_TEXTREL */		"(TEXTREL)",
	/* DT_JMPREL */			"(JMPREL)",
	/* DT_BIND_NOW */		"(BIND_NOW)",
	/* DT_INIT_ARRAY */		"(INIT_ARRAY)",
	/* DT_FINI_ARRAY */		"(FINI_ARRAY)",
	/* DT_INIT_ARRAYSZ */	"(INIT_ARRAYSZ)",
	/* DT_FINI_ARRAYSZ */	"(FINI_ARRAYSZ)",
	/* DT_RUNPATH */		"(RUNPATH)",
	/* DT_FLAGS */			"(FLAGS)",
	/* DT_ENCODING */		"(ENCODING)",
	/* DT_PREINIT_ARRAY */	"(PREINIT_ARRAY)",
	/* DT_PREINIT_ARRAYSZ */"(PREINIT_ARRAYSZ)",
};

static const char *i386Relocs[] = {
	/* R_386_NONE */		"R_386_NONE",
	/* R_386_32 */			"R_386_32",
	/* R_386_PC32 */		"R_386_PC32",
	/* R_386_GOT32 */		"R_386_GOT32",
	/* R_386_PLT32 */		"R_386_PLT32",
	/* R_386_COPY */		"R_386_COPY",
	/* R_386_GLOB_DAT */	"R_386_GLOB_DAT",
	/* R_386_JMP_SLOT */	"R_386_JMP_SLOT",
	/* R_386_RELATIVE */	"R_386_RELATIVE",
	/* R_386_GOTOFF */		"R_386_GOTOFF",
	/* R_386_GOTPC */		"R_386_GOTPC",
	/* R_386_32PLT */		"R_386_32PLT",
	/* R_386_TLS_TPOFF */	"R_386_TLS_TPOFF",
	/* R_386_TLS_IE */		"R_386_TLS_IE",
	/* R_386_TLS_GOTIE */	"R_386_TLS_GOTIE",
	/* R_386_TLS_LE */		"R_386_TLS_LE",
	/* R_386_TLS_GD */		"R_386_TLS_GD",
	/* R_386_TLS_LDM */		"R_386_TLS_LDM",
	/* R_386_16 */			"R_386_16",
	/* R_386_PC16 */		"R_386_PC16",
	/* R_386_8 */			"R_386_8",
	/* R_386_PC8 */			"R_386_PC8",
	/* R_386_TLS_GD_32 */	"R_386_TLS_GD_32",
	/* R_386_TLS_GD_PUSH */	"R_386_TLS_GD_PUSH",
	/* R_386_TLS_GD_CALL */	"R_386_TLS_GD_CALL",
	/* R_386_TLS_GD_POP */	"R_386_TLS_GD_POP",
	/* R_386_TLS_LDM_32 */	"R_386_TLS_LDM_32",
	/* R_386_TLS_LDM_PUSH */"R_386_TLS_LDM_PUSH",
	/* R_386_TLS_LDM_CALL */"R_386_TLS_LDM_CALL",
	/* R_386_TLS_LDM_POP */	"R_386_TLS_LDM_POP",
	/* R_386_TLS_LDO_32 */	"R_386_TLS_LDO_32",
	/* R_386_TLS_IE_32 */	"R_386_TLS_IE_32",
	/* R_386_TLS_LE_32 */	"R_386_TLS_LE_32",
	/* R_386_TLS_DTPMOD32 */"R_386_TLS_DTPMOD32",
	/* R_386_TLS_DTPOFF32 */"R_386_TLS_DTPOFF32",
	/* R_386_TLS_TPOFF32 */	"R_386_TLS_TPOFF32",
};

static sElfSHeader *getSecByName(const char *name);
static void loadShSyms(void);
static void loadDynSection(void);
static void printHeader(void);
static void printSectionHeader(void);
static const char *getSectionFlags(sElfWord flags);
static void printProgHeaders(void);
static const char *getProgFlags(sElfWord flags);
static void printDynSection(void);
static void printRelocations(void);
static void printRelocTable(sElfRel *rel,size_t relCount);
static void readat(off_t offset,void *buffer,size_t count);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-ahlSrd] <file>\n",name);
	exit(EXIT_FAILURE);
}

static int fd;
static sElfEHeader eheader;
static sElfAddr dynOff = 0;
static sElfDyn *dyn = NULL;
static sElfSym *dynsyms = NULL;
static char *shsymbols = NULL;
static char *dynstrtbl = NULL;

int main(int argc,char **argv) {
	int i;
	bool elfh = false,ph = false,sh = false,rel = false,dyns = false;
	char *file = NULL;

	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i],"-a") == 0)
			elfh = ph = sh = rel = dyns = true;
		else if(strcmp(argv[i],"-h") == 0)
			elfh = true;
		else if(strcmp(argv[i],"-l") == 0)
			ph = true;
		else if(strcmp(argv[i],"-S") == 0)
			sh = true;
		else if(strcmp(argv[i],"-r") == 0)
			rel = true;
		else if(strcmp(argv[i],"-d") == 0)
			dyns = true;
		else
			file = argv[i];
	}

	if(file == NULL || (elfh + ph + sh + rel + dyns) == 0)
		usage(argv[0]);
	fd = open(file,IO_READ);
	if(fd < 0)
		error("Unable to open %s",file);

	readat(0,&eheader,sizeof(sElfEHeader));
	loadShSyms();
	if(dyns || rel)
		loadDynSection();
	if(elfh)
		printHeader();
	if(sh)
		printSectionHeader();
	if(ph)
		printProgHeaders();
	if(dyns)
		printDynSection();
	if(rel)
		printRelocations();

	free(shsymbols);
	free(dyn);
	free(dynstrtbl);
	free(dynsyms);
	close(fd);
	return EXIT_SUCCESS;
}

static sElfSHeader *getSecByName(const char *name) {
	static sElfSHeader section[1];
	size_t i;
	uchar *datPtr = (uchar*)eheader.e_shoff;
	for(i = 0; i < eheader.e_shnum; datPtr += eheader.e_shentsize, i++) {
		readat((off_t)datPtr,section,sizeof(sElfSHeader));
		if(strcmp(shsymbols + section->sh_name,name) == 0)
			return section;
	}
	return NULL;
}

static void loadShSyms(void) {
	sElfSHeader sheader;
	uchar *datPtr = (uchar*)(eheader.e_shoff + eheader.e_shstrndx * eheader.e_shentsize);
	readat((off_t)datPtr,&sheader,sizeof(sElfSHeader));
	shsymbols = (char*)malloc(sheader.sh_size);
	if(shsymbols == NULL)
		error("Not enough mem for section-header-names");
	readat(sheader.sh_offset,shsymbols,sheader.sh_size);
}

static void loadDynSection(void) {
	sElfPHeader pheader;
	uchar *datPtr = (uchar*)(eheader.e_phoff);
	size_t i;
	/* find dynamic-section */
	for(i = 0; i < eheader.e_phnum; datPtr += eheader.e_phentsize, i++) {
		readat((off_t)datPtr,&pheader,sizeof(sElfPHeader));
		if(pheader.p_type == PT_DYNAMIC)
			break;
	}
	if(i != eheader.e_phnum) {
		/* load it */
		dynOff = pheader.p_offset;
		dyn = (sElfDyn*)malloc(pheader.p_filesz);
		if(!dyn)
			error("Not enough mem for dyn-section\n");
		readat(pheader.p_offset,dyn,pheader.p_filesz);

		/* load dyn strings */
		sElfSHeader *strtabSec = getSecByName(".dynstr");
		dynstrtbl = (char*)malloc(strtabSec->sh_size);
		if(!dynstrtbl)
			error("Not enough mem for dyn-strings!");
		readat(strtabSec->sh_offset,dynstrtbl,strtabSec->sh_size);

		/* load dyn syms */
		sElfSHeader *dynSymSec = getSecByName(".dynsym");
		if(dynSymSec == NULL)
			error("No dyn-sym-section!?");
		dynsyms = (sElfSym*)malloc(dynSymSec->sh_size);
		if(dynsyms == NULL)
			error("Not enough mem for dyn-syms");
		readat(dynSymSec->sh_offset,dynsyms,dynSymSec->sh_size);
	}
}

static void printHeader(void) {
	size_t i;
	printf("ELF Header:\n");
	printf("  Magic: ");
	for(i = 0; i < EI_NIDENT; i++)
		printf("%02x ",eheader.e_ident[i]);
	printf("\n");
	printf("  %-34s %s\n","Type:",
			eheader.e_type < ARRAY_SIZE(fileTypes) ? fileTypes[eheader.e_type] : "-Unknown-");
	printf("  %-34s %p\n","Entry point address:",eheader.e_entry);
	printf("  %-34s %lu (bytes into file)\n","Start of program headers:",eheader.e_phoff);
	printf("  %-34s %lu (bytes into file)\n","Start of section headers:",eheader.e_shoff);
	printf("  %-34s %zu (bytes)\n","Size of this header:",sizeof(sElfEHeader));
	printf("  %-34s %hu (bytes)\n","Size of program headers:",eheader.e_phentsize);
	printf("  %-34s %hu\n","Number of program headers:",eheader.e_phnum);
	printf("  %-34s %hu (bytes)\n","Size of section headers:",eheader.e_shentsize);
	printf("  %-34s %hu\n","Number of section headers:",eheader.e_shnum);
	printf("  %-34s %hu\n","Section header string table index:",eheader.e_shstrndx);
	printf("\n");
}

static void printSectionHeader(void) {
	sElfSHeader sheader;
	uchar *datPtr;
	size_t i;

	/* print sections */
	printf("Section Headers:\n");
	printf("  Nr %-18s %-8s %-"POFFW"s %-6s %-6s %-2s %-3s %-2s %-3s %-2s\n",
			"Name","Type","Addr","Off","Size","ES","Flg","Lk","Inf","Al");
	datPtr = (uchar*)(eheader.e_shoff);
	for(i = 0; i < eheader.e_shnum; datPtr += eheader.e_shentsize, i++) {
		readat((off_t)datPtr,&sheader,sizeof(sElfSHeader));
		printf("  %2zu %-18s %-8s %0"POFFW"x %06x %06x %02x %3s %2d %3d %2d\n",
				i,shsymbols + sheader.sh_name,
				sheader.sh_type < ARRAY_SIZE(secTypes) ? secTypes[sheader.sh_type] : "-Unknown-",
				sheader.sh_addr,sheader.sh_offset,sheader.sh_size,sheader.sh_entsize,
				getSectionFlags(sheader.sh_flags),sheader.sh_link,sheader.sh_info,
				sheader.sh_addralign);
	}
	printf("Key to Flags:\n");
	printf("  W (write), A (alloc), X (execute), M (merge), S (strings)\n");
	printf("  I (info), L (link order), G (group), x (unknown)\n");
	printf("\n");
}

static const char *getSectionFlags(sElfWord flags) {
	static char str[9];
	char *s = str;
	if(flags & SHF_WRITE)
		*s++ = 'W';
	if(flags & SHF_ALLOC)
		*s++ = 'A';
	if(flags & SHF_EXECINSTR)
		*s++ = 'X';
	if(flags & SHF_MERGE)
		*s++ = 'M';
	if(flags & SHF_STRINGS)
		*s++ = 'S';
	if(flags & SHF_INFO_LINK)
		*s++ = 'I';
	if(flags & SHF_LINK_ORDER)
		*s++ = 'L';
	if(flags & SHF_GROUP)
		*s++ = 'G';
	if(s == str && flags)
		*s++ = 'x';
	*s = '\0';
	return str;
}

static void printProgHeaders(void) {
	sElfPHeader pheader;
	uchar *datPtr = (uchar*)(eheader.e_phoff);
	size_t i;

	printf("Program Headers:\n");
	printf("  %-14s %-8s %-"PADDRW"s   %-7s %-7s %-3s %-4s\n",
			"Type","Offset","VirtAddr","FileSiz","MemSiz","Flg","Align");
	printf("  %-23s %-"PADDRW"s\n","","PhysAddr");
	for(i = 0; i < eheader.e_phnum; datPtr += eheader.e_phentsize, i++) {
		readat((off_t)datPtr,&pheader,sizeof(sElfPHeader));
		printf("  %-14s %#06lx %#0"PADDRW"lx %#05lx %#05lx %3s %#04lx\n",
				pheader.p_type < ARRAY_SIZE(progTypes) ? progTypes[pheader.p_type] : "-Unknown-",
				pheader.p_offset,pheader.p_vaddr,pheader.p_filesz,
				pheader.p_memsz,getProgFlags(pheader.p_flags),pheader.p_align);
		printf("  %-23s %#0"PADDRW"lx\n","",pheader.p_paddr);
	}
	printf("\n");
}

static const char *getProgFlags(sElfWord flags) {
	static char str[4];
	str[0] = (flags & PF_R) ? 'R' : ' ';
	str[1] = (flags & PF_W) ? 'W' : ' ';
	str[2] = (flags & PF_X) ? 'E' : ' ';
	return str;
}

static void printDynSection(void) {
	sElfDyn *dwalk = dyn;
	size_t eCount = 0;
	if(dyn == NULL) {
		printf("There is no dynamic section in this file.\n\n");
		return;
	}

	/* count entries */
	while(dwalk->d_tag != DT_NULL) {
		eCount++;
		dwalk++;
	}

	/* print entries */
	dwalk = dyn;
	printf("Dynamic section at offset %#x contains %d entries:\n",dynOff,eCount);
	printf("  %-10s %-28s %s\n","Tag","Type","Name/Value");
	while(dwalk->d_tag != DT_NULL) {
		printf(" %#08x %-28s",dwalk->d_tag,
			dwalk->d_tag < (int)ARRAY_SIZE(relocTypes) ? relocTypes[dwalk->d_tag] : "-Unknown-");
		switch(dwalk->d_tag) {
			case DT_SONAME:
			case DT_NEEDED:
				printf("Shared library: [%s]\n",dynstrtbl + dwalk->d_un.d_val);
				break;
			case DT_INIT:
			case DT_FINI:
			case DT_HASH:
			case DT_STRTAB:
			case DT_SYMTAB:
			case DT_PLTGOT:
			case DT_JMPREL:
			case DT_REL:
				printf("%#x\n",dwalk->d_un.d_ptr);
				break;
			case DT_STRSZ:
			case DT_SYMENT:
			case DT_PLTRELSZ:
			case DT_RELSZ:
			case DT_RELENT:
				printf("%d (bytes)\n",dwalk->d_un.d_val);
				break;
			default:
				printf("?\n");
				break;
		}
		dwalk++;
	}
	printf("\n");
}

static void printRelocations(void) {
	sElfSHeader *relSec = getSecByName(".rel.dyn");
	size_t relCount = relSec ? relSec->sh_size / sizeof(sElfRel) : 0;
	sElfRel *rel;
	if(relSec == NULL || relCount == 0) {
		printf("There are no relocations in this file.\n\n");
		return;
	}

	/* relocations */
	rel = (sElfRel*)malloc(relCount * sizeof(sElfRel));
	if(!rel)
		error("Not enough mem for relocations");
	readat(relSec->sh_offset,rel,relCount * sizeof(sElfRel));
	/* TODO there are different relocation-sections */
	printf("Relocation section '.rel.dyn' at offset %#x contains %d entries:\n",
			relSec->sh_offset,relCount);
	printf(" %-10s %-7s %-15s %-10s %s\n","Offset","Info","Type","Sym.Value","Sym. Name");
	printRelocTable(rel,relCount);
	printf("\n");
	free(rel);

	/* jump-relocs */
	relSec = getSecByName(".rel.plt");
	relCount = relSec->sh_size / sizeof(sElfRel);
	rel = (sElfRel*)malloc(relCount * sizeof(sElfRel));
	if(!rel)
		error("Not enough mem for jump-relocations");
	readat(relSec->sh_offset,rel,relCount * sizeof(sElfRel));
	printf("Relocation section '.rel.plt' at offset %#x contains %d entries:\n",
			relSec->sh_offset,relCount);
	printf(" %-10s %-7s %-15s %-10s %s\n","Offset","Info","Type","Sym.Value","Sym. Name");
	printRelocTable(rel,relCount);
	printf("\n");
	free(rel);
}

static void printRelocTable(sElfRel *rel,size_t relCount) {
	char symcopy[MAX_SYM_LEN];
	size_t i;
	for(i = 0; i < relCount; i++) {
		uint relType = ELF32_R_TYPE(rel[i].r_info);
		uint symIndex = ELF32_R_SYM(rel[i].r_info);
		printf("%08x  %08x %-16s",rel[i].r_offset,rel[i].r_info,
			relType < ARRAY_SIZE(i386Relocs) ? i386Relocs[relType] : "-Unknown-");
		if(symIndex != 0) {
			sElfSym *sym = dynsyms + symIndex;
			const char *symName = dynstrtbl + sym->st_name;
			size_t len = 0;
			if(symName) {
				len = MIN(strlen(symName),MAX_SYM_LEN - 1);
				strncpy(symcopy,symName,len);
			}
			symcopy[len] = '\0';
			printf(" %08x   %-25s",sym->st_value,symcopy);
		}
		printf("\n");
	}
}

static void readat(off_t offset,void *buffer,size_t count) {
	if(seek(fd,offset,SEEK_SET) < 0)
		error("Unable to seek to %x",offset);
	if(RETRY(read(fd,buffer,count)) != (ssize_t)count)
		error("Unable to read %d bytes @ %x",count,offset);
}
