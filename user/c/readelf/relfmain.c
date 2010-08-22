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
#include <sys/task/elf.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <stdlib.h>
#include <stdio.h>

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

static Elf32_Shdr *getSecByName(const char *name);
static void loadShSyms(void);
static void loadDynSection(void);
static void printHeader(void);
static void printSectionHeader(void);
static const char *getSectionFlags(Elf32_Word flags);
static void printProgHeaders(void);
static const char *getProgFlags(Elf32_Word flags);
static void printDynSection(void);
static u32 getDynEntry(Elf32_Sword tag);
static void printRelocations(void);
static void printRelocTable(Elf32_Rel *rel,s32 relCount);
static void readat(u32 offset,void *buffer,u32 count);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-ahlSrd] <file>\n",name);
	exit(EXIT_FAILURE);
}

static tFD fd;
static Elf32_Ehdr eheader;
static Elf32_Addr dynOff = 0;
static Elf32_Dyn *dyn = NULL;
static Elf32_Sym *dynsyms = NULL;
static char *shsymbols = NULL;
static char *dynstrtbl = NULL;

int main(int argc,char **argv) {
	char path[MAX_PATH_LEN];
	s32 i;
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
	abspath(path,sizeof(path),file);
	fd = open(path,IO_READ);
	if(fd < 0)
		error("Unable to open %s",path);

	readat(0,&eheader,sizeof(Elf32_Ehdr));
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

static Elf32_Shdr *getSecByName(const char *name) {
	static Elf32_Shdr section[1];
	s32 i;
	u8 *datPtr = (u8*)eheader.e_shoff;
	for(i = 0; i < eheader.e_shnum; datPtr += eheader.e_shentsize, i++) {
		readat((u32)datPtr,section,sizeof(Elf32_Shdr));
		if(strcmp(shsymbols + section->sh_name,name) == 0)
			return section;
	}
	return NULL;
}

static void loadShSyms(void) {
	Elf32_Shdr sheader;
	u8 *datPtr = (u8*)(eheader.e_shoff + eheader.e_shstrndx * eheader.e_shentsize);
	readat((u32)datPtr,&sheader,sizeof(Elf32_Shdr));
	shsymbols = (char*)malloc(sheader.sh_size);
	if(shsymbols == NULL)
		error("Not enough mem for section-header-names");
	readat(sheader.sh_offset,shsymbols,sheader.sh_size);
}

static void loadDynSection(void) {
	Elf32_Phdr pheader;
	u32 textOffset = 0xFFFFFFFF;
	u8 *datPtr = (u8*)(eheader.e_phoff);
	s32 i;
	/* find dynamic-section */
	for(i = 0; i < eheader.e_phnum; datPtr += eheader.e_phentsize, i++) {
		readat((u32)datPtr,&pheader,sizeof(Elf32_Phdr));
		if(textOffset == 0xFFFFFFFF && pheader.p_type == PT_LOAD)
			textOffset = pheader.p_offset - pheader.p_vaddr;
		if(pheader.p_type == PT_DYNAMIC)
			break;
	}
	if(i != eheader.e_phnum) {
		/* load it */
		dynOff = pheader.p_offset;
		dyn = (Elf32_Dyn*)malloc(pheader.p_filesz);
		if(!dyn)
			error("Not enough mem for dyn-section\n");
		readat(pheader.p_offset,dyn,pheader.p_filesz);

		/* load dyn strings */
		u32 strtblSize = getDynEntry(DT_STRSZ);
		dynstrtbl = (char*)malloc(strtblSize);
		if(!dynstrtbl)
			error("Not enough mem for dyn-strings!");
		readat(getDynEntry(DT_STRTAB) + textOffset,dynstrtbl,strtblSize);

		/* load dyn syms */
		Elf32_Shdr *dynSymSec = getSecByName(".dynsym");
		if(dynSymSec == NULL)
			error("No dyn-sym-section!?");
		dynsyms = (Elf32_Sym*)malloc(dynSymSec->sh_size);
		if(dynsyms == NULL)
			error("Not enough mem for dyn-syms");
		readat(getDynEntry(DT_SYMTAB),dynsyms,dynSymSec->sh_size);
	}
}

static void printHeader(void) {
	s32 i;
	printf("ELF Header:\n");
	printf("  Magic: ");
	for(i = 0; i < EI_NIDENT; i++)
		printf("%02x ",eheader.e_ident.chars[i]);
	printf("\n");
	printf("  %-34s %s\n","Type:",
			eheader.e_type < ARRAY_SIZE(fileTypes) ? fileTypes[eheader.e_type] : "-Unknown-");
	printf("  %-34s %#x\n","Entry point address:",eheader.e_entry);
	printf("  %-34s %hd (bytes into file)\n","Start of program headers:",eheader.e_phoff);
	printf("  %-34s %hd (bytes into file)\n","Start of section headers:",eheader.e_shoff);
	printf("  %-34s %d (bytes)\n","Size of this header:",sizeof(Elf32_Ehdr));
	printf("  %-34s %hd (bytes)\n","Size of program headers:",eheader.e_phentsize);
	printf("  %-34s %hd\n","Number of program headers:",eheader.e_phnum);
	printf("  %-34s %hd (bytes)\n","Size of section headers:",eheader.e_shentsize);
	printf("  %-34s %hd\n","Number of section headers:",eheader.e_shnum);
	printf("  %-34s %hd\n","Section header string table index:",eheader.e_shstrndx);
	printf("\n");
}

static void printSectionHeader(void) {
	Elf32_Shdr sheader;
	u8 *datPtr;
	s32 i;

	/* print sections */
	printf("Section Headers:\n");
	printf("  [Nr] %-17s %-15s %-8s %-6s %-6s %-2s %-3s %-2s %-3s %-2s\n",
			"Name","Type","Addr","Off","Size","ES","Flg","Lk","Inf","Al");
	datPtr = (u8*)(eheader.e_shoff);
	for(i = 0; i < eheader.e_shnum; datPtr += eheader.e_shentsize, i++) {
		readat((u32)datPtr,&sheader,sizeof(Elf32_Shdr));
		printf("  [%2d] %-17s %-15s %08x %06x %06x %02x %3s %2d %3d %2d\n",
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

static const char *getSectionFlags(Elf32_Word flags) {
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
	Elf32_Phdr pheader;
	u8 *datPtr = (u8*)(eheader.e_phoff);
	s32 i;

	printf("Program Headers:\n");
	printf("  %-14s %-8s %-10s %-10s %-7s %-7s %-3s %-4s\n",
			"Type","Offset","VirtAddr","PhysAddr","FileSiz","MemSiz","Flg","Align");
	for(i = 0; i < eheader.e_phnum; datPtr += eheader.e_phentsize, i++) {
		readat((u32)datPtr,&pheader,sizeof(Elf32_Phdr));
		printf("  %-14s %#06x %#08x %#08x %#05x %#05x %3s %#04x\n",
				pheader.p_type < ARRAY_SIZE(progTypes) ? progTypes[pheader.p_type] : "-Unknown-",
				pheader.p_offset,pheader.p_vaddr,pheader.p_paddr,pheader.p_filesz,
				pheader.p_memsz,getProgFlags(pheader.p_flags),pheader.p_align);
	}
	printf("\n");
}

static const char *getProgFlags(Elf32_Word flags) {
	static char str[4];
	str[0] = (flags & PF_R) ? 'R' : ' ';
	str[1] = (flags & PF_W) ? 'W' : ' ';
	str[2] = (flags & PF_X) ? 'E' : ' ';
	return str;
}

static void printDynSection(void) {
	Elf32_Dyn *dwalk = dyn;
	u32 eCount = 0;
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
			dwalk->d_tag < (s32)ARRAY_SIZE(relocTypes) ? relocTypes[dwalk->d_tag] : "-Unknown-");
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

static u32 getDynEntry(Elf32_Sword tag) {
	Elf32_Dyn *dwalk = dyn;
	if(dyn == NULL)
		return 0;
	while(dwalk->d_tag != DT_NULL) {
		if(dwalk->d_tag == tag)
			return dwalk->d_un.d_val;
		dwalk++;
	}
	return 0;
}

static void printRelocations(void) {
	u32 addr = getDynEntry(DT_REL);
	u32 relCount = getDynEntry(DT_RELSZ) / sizeof(Elf32_Rel);
	Elf32_Rel *rel;
	if(addr == 0 || relCount == 0) {
		printf("There are no relocations in this file.\n\n");
		return;
	}

	/* relocations */
	rel = (Elf32_Rel*)malloc(relCount * sizeof(Elf32_Rel));
	if(!rel)
		error("Not enough mem for relocations");
	readat(addr,rel,relCount * sizeof(Elf32_Rel));
	/* TODO there are different relocation-sections */
	printf("Relocation section '.rel.dyn' at offset %#x contains %d entries:\n",addr,relCount);
	printf(" %-10s %-7s %-15s %-10s %s\n","Offset","Info","Type","Sym.Value","Sym. Name");
	printRelocTable(rel,relCount);
	printf("\n");
	free(rel);

	/* jump-relocs */
	addr = getDynEntry(DT_JMPREL);
	relCount = getDynEntry(DT_PLTRELSZ) / sizeof(Elf32_Rel);
	rel = (Elf32_Rel*)malloc(relCount * sizeof(Elf32_Rel));
	if(!rel)
		error("Not enough mem for jump-relocations");
	readat(addr,rel,relCount * sizeof(Elf32_Rel));
	printf("Relocation section '.rel.plt' at offset %#x contains %d entries:\n",addr,relCount);
	printf(" %-10s %-7s %-15s %-10s %s\n","Offset","Info","Type","Sym.Value","Sym. Name");
	printRelocTable(rel,relCount);
	printf("\n");
	free(rel);
}

static void printRelocTable(Elf32_Rel *rel,s32 relCount) {
	char symcopy[MAX_SYM_LEN];
	s32 i;
	for(i = 0; i < relCount; i++) {
		u32 relType = ELF32_R_TYPE(rel[i].r_info);
		u32 symIndex = ELF32_R_SYM(rel[i].r_info);
		printf("%08x  %08x %-16s",rel[i].r_offset,rel[i].r_info,
			relType < ARRAY_SIZE(i386Relocs) ? i386Relocs[relType] : "-Unknown-");
		if(symIndex != 0) {
			Elf32_Sym *sym = dynsyms + symIndex;
			const char *symName = dynstrtbl + sym->st_name;
			u32 len = MIN(strlen(symName),MAX_SYM_LEN - 1);
			strncpy(symcopy,symName,len);
			symcopy[len] = '\0';
			printf(" %08x   %-25s",sym->st_value,symcopy);
		}
		printf("\n");
	}
}

static void readat(u32 offset,void *buffer,u32 count) {
	if(seek(fd,(u32)offset,SEEK_SET) < 0)
		error("Unable to seek to %x",offset);
	if(RETRY(read(fd,buffer,count)) != (s32)count)
		error("Unable to read %d bytes @ %x",count,offset);
}
