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
#include <dirent.h>
#include <sys/io.h>
#include <sys/elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if ELF_TYPE == 64
#	define POFFW	"16"
#	define PADDRW	"16"
#	define INFOW	"12"
#	define SYMVALW	"16"
#else
#	define POFFW	"8"
#	define PADDRW	"8"
#	define INFOW	"8"
#	define SYMVALW	"8"
#endif

#define MAX_SYM_LEN		40

typedef struct {
	const char *name;
	ulong tag;
} sDynType;

typedef struct {
	int id;
	const char *name;
} sMachineEntry;

static const char *elfTypes[] = {
	/* ELFCLASSNONE */		"NONE",
	/* ELFCLASS32 */		"ELF32",
	/* ELFCLASS64 */		"ELF64",
};

static const char *dataTypes[] = {
	/* ELFDATANONE */		"NONE",
	/* ELFDATA2LSB */		"2's complement, little endian",
	/* ELFDATA2MSB */		"2's complement, big endian",
};

static const char *fileTypes[] = {
	/* ET_NONE */			"NONE (No file type)",
	/* ET_REL */			"REL (Relocatable file)",
	/* ET_EXEC */			"EXEC (Executable file)",
	/* ET_DYN */			"DYN (Shared object file)",
	/* ET_CORE */			"CORE (Core file)",
};

static const char *osABIs[] = {
	/* ELFOSABI_NONE */			"UNIX System V ABI",
	/* ELFOSABI_SYSV */			"Alias",
	/* ELFOSABI_HPUX */			"HP-UX",
	/* ELFOSABI_NETBSD */		"NetBSD",
	/* ELFOSABI_LINUX */		"Linux",
	/* ELFOSABI_SOLARIS */		"Solaris",
	/* ELFOSABI_AIX */			"IBM AIX",
	/* ELFOSABI_IRIX */			"SGI Irix",
	/* ELFOSABI_FREEBSD */		"FreeBSD",
	/* ELFOSABI_TRU64 */		"Compaq TRU64 UNIX",
	/* ELFOSABI_MODESTO */		"Novell Modesto",
	/* ELFOSABI_OPENBSD */		"OpenBSD",
	/* ELFOSABI_ARM */			"ARM",
	/* ELFOSABI_STANDALONE */	"Standalone (embedded) application"
};

static const sMachineEntry machines[] = {
	/* EM_NONE */			{0, "No machine"},
	/* EM_M32 */			{1, "AT&T WE 32100"},
	/* EM_SPARC */			{2, "SUN SPARC"},
	/* EM_386 */			{3, "Intel 80386"},
	/* EM_68K */			{4, "Motorola m68k family"},
	/* EM_88K */			{5, "Motorola m88k family"},
	/* EM_860 */			{7, "Intel 80860"},
	/* EM_MIPS */			{8, "MIPS R3000 big-endian"},
	/* EM_S370 */			{9, "IBM System/370"},
	/* EM_MIPS_RS3_LE */	{10, "MIPS R3000 little-endian"},

	/* EM_PARISC */			{15, "HPPA"},
	/* EM_VPP500 */			{17, "Fujitsu VPP500"},
	/* EM_SPARC32PLUS */	{18, "Sun's v8plus"},
	/* EM_960 */			{19, "Intel 80960"},
	/* EM_PPC */			{20, "PowerPC"},
	/* EM_PPC64 */			{21, "PowerPC 64-bit"},
	/* EM_S390 */			{22, "IBM S390"},

	/* EM_V800 */			{36, "NEC V800 series"},
	/* EM_FR20 */			{37, "Fujitsu FR20"},
	/* EM_RH32 */			{38, "TRW RH-32"},
	/* EM_RCE */			{39, "Motorola RCE"},
	/* EM_ARM */			{40, "ARM"},
	/* EM_FAKE_ALPHA */		{41, "Digital Alpha"},
	/* EM_SH */				{42, "Hitachi SH"},
	/* EM_SPARCV9 */		{43, "SPARC v9 64-bit"},
	/* EM_TRICORE */		{44, "Siemens Tricore"},
	/* EM_ARC */			{45, "Argonaut RISC Core"},
	/* EM_H8_300 */			{46, "Hitachi H8/300"},
	/* EM_H8_300H */		{47, "Hitachi H8/300H"},
	/* EM_H8S */			{48, "Hitachi H8S"},
	/* EM_H8_500 */			{49, "Hitachi H8/500"},
	/* EM_IA_64 */			{50, "Intel Merced"},
	/* EM_MIPS_X */			{51, "Stanford MIPS-X"},
	/* EM_COLDFIRE */		{52, "Motorola Coldfire"},
	/* EM_68HC12 */			{53, "Motorola M68HC12"},
	/* EM_MMA */			{54, "Fujitsu MMA Multimedia Accelerator"},
	/* EM_PCP */			{55, "Siemens PCP"},
	/* EM_NCPU */			{56, "Sony nCPU embeeded RISC"},
	/* EM_NDR1 */			{57, "Denso NDR1 microprocessor"},
	/* EM_STARCORE */		{58, "Motorola Start*Core processor"},
	/* EM_ME16 */			{59, "Toyota ME16 processor"},
	/* EM_ST100 */			{60, "STMicroelectronic ST100 processor"},
	/* EM_TINYJ */			{61, "Advanced Logic Corp. Tinyj emb.fam"},
	/* EM_X86_64 */			{62, "AMD x86-64 architecture"},
	/* EM_PDSP */			{63, "Sony DSP Processor"},

	/* EM_FX66 */			{66, "Siemens FX66 microcontroller"},
	/* EM_ST9PLUS */		{67, "STMicroelectronics ST9+ 8/16 mc"},
	/* EM_ST7 */			{68, "STmicroelectronics ST7 8 bit mc"},
	/* EM_68HC16 */			{69, "Motorola MC68HC16 microcontroller"},
	/* EM_68HC11 */			{70, "Motorola MC68HC11 microcontroller"},
	/* EM_68HC08 */			{71, "Motorola MC68HC08 microcontroller"},
	/* EM_68HC05 */			{72, "Motorola MC68HC05 microcontroller"},
	/* EM_SVX */			{73, "Silicon Graphics SVx"},
	/* EM_ST19 */			{74, "STMicroelectronics ST19 8 bit mc"},
	/* EM_VAX */			{75, "Digital VAX"},
	/* EM_CRIS */			{76, "Axis Communications 32-bit embedded processor"},
	/* EM_JAVELIN */		{77, "Infineon Technologies 32-bit embedded processor"},
	/* EM_FIREPATH */		{78, "Element 14 64-bit DSP Processor"},
	/* EM_ZSP */			{79, "LSI Logic 16-bit DSP Processor"},
	/* EM_MMIX */			{80, "Donald Knuth's educational 64-bit processor"},
	/* EM_HUANY */			{81, "Harvard University machine-independent object files"},
	/* EM_PRISM */			{82, "SiTera Prism"},
	/* EM_AVR */			{83, "Atmel AVR 8-bit microcontroller"},
	/* EM_FR30 */			{84, "Fujitsu FR30"},
	/* EM_D10V */			{85, "Mitsubishi D10V"},
	/* EM_D30V */			{86, "Mitsubishi D30V"},
	/* EM_V850 */			{87, "NEC v850"},
	/* EM_M32R */			{88, "Mitsubishi M32R"},
	/* EM_MN10300 */		{89, "Matsushita MN10300"},
	/* EM_MN10200 */		{90, "Matsushita MN10200"},
	/* EM_PJ */				{91, "picoJava"},
	/* EM_OPENRISC */		{92, "OpenRISC 32-bit embedded processor"},
	/* EM_ARC_A5 */			{93, "ARC Cores Tangent-A5"},
	/* EM_XTENSA */			{94, "Tensilica Xtensa Architecture"},

	/* EM_ALPHA */			{0x9026, "Alpha"},
	/* EM_ECO32 */			{0xEC32, "ECO32"},
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

static sDynType dynTypes[] = {
	{"(NULL)",				DT_NULL},
	{"(NEEDED)",			DT_NEEDED},
	{"(PLTRELSZ)",			DT_PLTRELSZ},
	{"(PLTGOT)",			DT_PLTGOT},
	{"(HASH)",				DT_HASH},
	{"(STRTAB)",			DT_STRTAB},
	{"(SYMTAB)",			DT_SYMTAB},
	{"(RELA)",				DT_RELA},
	{"(RELASZ)",			DT_RELASZ},
	{"(RELAENT)",			DT_RELAENT},
	{"(STRSZ)",				DT_STRSZ},
	{"(SYMENT)",			DT_SYMENT},
	{"(INIT)",				DT_INIT},
	{"(FINI)",				DT_FINI},
	{"(SONAME)",			DT_SONAME},
	{"(RPATH)",				DT_RPATH},
	{"(SYMBOLIC)",			DT_SYMBOLIC},
	{"(REL)",				DT_REL},
	{"(RELSZ)",				DT_RELSZ},
	{"(RELENT)",			DT_RELENT},
	{"(PLTREL)",			DT_PLTREL},
	{"(DEBUG)",				DT_DEBUG},
	{"(TEXTREL)",			DT_TEXTREL},
	{"(JMPREL)",			DT_JMPREL},
	{"(BIND_NOW)",			DT_BIND_NOW},
	{"(INIT_ARRAY)",		DT_INIT_ARRAY},
	{"(FINI_ARRAY)",		DT_FINI_ARRAY},
	{"(INIT_ARRAYSZ)",		DT_INIT_ARRAYSZ},
	{"(FINI_ARRAYSZ)",		DT_FINI_ARRAYSZ},
	{"(RUNPATH)",			DT_RUNPATH},
	{"(FLAGS)",				DT_FLAGS},
	{"(ENCODING)",			DT_ENCODING},
	{"(PREINIT_ARRAY)",		DT_PREINIT_ARRAY},
	{"(PREINIT_ARRAYSZ)",	DT_PREINIT_ARRAYSZ},
	{"(VERSYM)",			DT_VERSYM},
	{"(RELACOUNT)",			DT_RELACOUNT},
	{"(RELCOUNT)",			DT_RELCOUNT},
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

static const char *x86_64Relocs[] = {
	/* R_X86_64_NONE */			"X86_64_NONE",
	/* R_X86_64_64 */			"X86_64_64",
	/* R_X86_64_PC32 */			"X86_64_PC32",
	/* R_X86_64_GOT32 */		"X86_64_GOT32",
	/* R_X86_64_PLT32 */		"X86_64_PLT32",
	/* R_X86_64_COPY */			"X86_64_COPY",
	/* R_X86_64_GLOB_DAT */		"X86_64_GLOB_DAT",
	/* R_X86_64_JUMP_SLOT */	"X86_64_JUMP_SLOT",
	/* R_X86_64_RELATIVE */		"X86_64_RELATIVE",
	/* R_X86_64_GOTPCREL */		"X86_64_GOTPCREL",
	/* R_X86_64_32 */			"X86_64_32",
	/* R_X86_64_32S */			"X86_64_32S",
	/* R_X86_64_16 */			"X86_64_16",
	/* R_X86_64_PC16 */			"X86_64_PC16",
	/* R_X86_64_8 */			"X86_64_8",
	/* R_X86_64_PC8 */			"X86_64_PC8",
	/* R_X86_64_DTPMOD64 */		"X86_64_DTPMOD64",
	/* R_X86_64_DTPOFF64 */		"X86_64_DTPOFF64",
	/* R_X86_64_TPOFF64 */		"X86_64_TPOFF64",
	/* R_X86_64_TLSGD */		"X86_64_TLSGD",
	/* R_X86_64_TLSLD */		"X86_64_TLSLD",
	/* R_X86_64_DTPOFF32 */		"X86_64_DTPOFF32",
	/* R_X86_64_GOTTPOFF */		"X86_64_GOTTPOFF",
	/* R_X86_64_TPOFF32 */		"X86_64_TPOFF32",
};

static sElfSHeader *getSecByName(const char *name);
static void loadShSyms(void);
static void loadDynSection(void);
static void printHeader(void);
static void printSectionHeader(void);
static const char *getSectionFlags(ElfWord flags);
static void printProgHeaders(void);
static const char *getProgFlags(ElfWord flags);
static void printDynSection(void);
static void printRelocations(void);
static bool printRelocationsOf(const char *section,int type,const char **relTypes,size_t typeCount);
static void printRelTable(sElfRel *rel,size_t relCount,const char **relTypes,size_t typeCount);
static void printRelaTable(sElfRela *rel,size_t relCount,const char **relTypes,size_t typeCount);
static void readat(off_t offset,void *buffer,size_t count);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-ahlSrd] <file>\n",name);
	fprintf(stderr,"    -a: print all\n");
	fprintf(stderr,"    -h: print header\n");
	fprintf(stderr,"    -l: print program headers\n");
	fprintf(stderr,"    -S: print section headers\n");
	fprintf(stderr,"    -r: print relocations\n");
	fprintf(stderr,"    -d: print dynamic section\n");
	exit(EXIT_FAILURE);
}

static int fd;
static sElfEHeader eheader;
static ElfAddr dynOff = 0;
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

	if(file == NULL || !(elfh || ph || sh || rel || dyns))
		usage(argv[0]);
	fd = open(file,O_RDONLY);
	if(fd < 0)
		error("Unable to open %s",file);

	readat(0,&eheader,sizeof(sElfEHeader));
	if(memcmp(eheader.e_ident,ELFMAG,sizeof(ELFMAG) - 1) != 0) {
		printe("Invalid ELF-magic. This doesn't seem to be an ELF-binary\n");
		return EXIT_FAILURE;
	}
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
		sElfSHeader *strtabSec,*dynSymSec;
		/* load it */
		dynOff = pheader.p_offset;
		dyn = (sElfDyn*)malloc(pheader.p_filesz);
		if(!dyn)
			error("Not enough mem for dyn-section\n");
		readat(pheader.p_offset,dyn,pheader.p_filesz);

		/* load dyn strings */
		strtabSec = getSecByName(".dynstr");
		dynstrtbl = (char*)malloc(strtabSec->sh_size);
		if(!dynstrtbl)
			error("Not enough mem for dyn-strings!");
		readat(strtabSec->sh_offset,dynstrtbl,strtabSec->sh_size);

		/* load dyn syms */
		dynSymSec = getSecByName(".dynsym");
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
	printf("  %-34s %s\n","Class:",
		eheader.e_ident[EI_CLASS] < ARRAY_SIZE(elfTypes) ? elfTypes[eheader.e_ident[EI_CLASS]] : "Unknown");
	printf("  %-34s %s\n","Data:",
		eheader.e_ident[EI_DATA] < ARRAY_SIZE(dataTypes) ? dataTypes[eheader.e_ident[EI_DATA]] : "Unknown");
	printf("  %-34s %d\n","Version:",eheader.e_ident[EI_VERSION]);
	printf("  %-34s %s\n","OS/ABI:",
		eheader.e_ident[EI_OSABI] < ARRAY_SIZE(osABIs) ? osABIs[eheader.e_ident[EI_OSABI]] : "Unknown");
	printf("  %-34s %d\n","ABI Version:",eheader.e_ident[EI_ABIVERSION]);
	printf("  %-34s %s\n","Type:",
		eheader.e_type < ARRAY_SIZE(fileTypes) ? fileTypes[eheader.e_type] : "-Unknown-");
	for(size_t i = 0; i < ARRAY_SIZE(machines); ++i) {
		if(machines[i].id == eheader.e_machine) {
			printf("  %-34s %s\n","Machine:",machines[i].name);
			break;
		}
	}
	printf("  %-34s %d\n","Version:",eheader.e_version);
	printf("  %-34s %p\n","Entry point address:",(void*)eheader.e_entry);
	printf("  %-34s %lu (bytes into file)\n","Start of program headers:",(ulong)eheader.e_phoff);
	printf("  %-34s %lu (bytes into file)\n","Start of section headers:",(ulong)eheader.e_shoff);
	printf("  %-34s %#x\n","Flags:",eheader.e_flags);
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

static const char *getSectionFlags(ElfWord flags) {
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
				(ulong)pheader.p_offset,(ulong)pheader.p_vaddr,(ulong)pheader.p_filesz,
				(ulong)pheader.p_memsz,getProgFlags(pheader.p_flags),(ulong)pheader.p_align);
		printf("  %-23s %#0"PADDRW"lx\n","",(ulong)pheader.p_paddr);
	}
	printf("\n");
}

static const char *getProgFlags(ElfWord flags) {
	static char str[4];
	str[0] = (flags & PF_R) ? 'R' : ' ';
	str[1] = (flags & PF_W) ? 'W' : ' ';
	str[2] = (flags & PF_X) ? 'E' : ' ';
	return str;
}

static sDynType *getDynType(ulong tag) {
	for(size_t i = 0; i < ARRAY_SIZE(dynTypes); ++i) {
		if(dynTypes[i].tag == tag)
			return dynTypes + i;
	}
	return NULL;
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
	printf(" %-10s %-28s %s\n","Tag","Type","Name/Value");
	while(dwalk->d_tag != DT_NULL) {
		sDynType *relType = getDynType(dwalk->d_tag);
		printf(" %#08x %-28s ",dwalk->d_tag,relType ? relType->name : "-Unknown-");
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
			case DT_RELA:
			case DT_DEBUG:
				printf("%#x\n",dwalk->d_un.d_ptr);
				break;

			case DT_RELCOUNT:
			case DT_RELACOUNT:
				printf("%d\n",dwalk->d_un.d_val);
				break;

			case DT_STRSZ:
			case DT_SYMENT:
			case DT_PLTRELSZ:
			case DT_RELSZ:
			case DT_RELENT:
			case DT_RELASZ:
			case DT_RELAENT:
				printf("%d (bytes)\n",dwalk->d_un.d_val);
				break;

			case DT_PLTREL:
				printf("%s\n",dwalk->d_un.d_val == DT_REL ? "REL" : "RELA");
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
	const char **relTypes;
	size_t typeCount;
	if(eheader.e_machine == EM_386) {
		relTypes = i386Relocs;
		typeCount = ARRAY_SIZE(i386Relocs);
	}
	else if(eheader.e_machine == EM_X86_64) {
		relTypes = x86_64Relocs;
		typeCount = ARRAY_SIZE(x86_64Relocs);
	}
	else {
		printf("There are no relocations in this file.\n\n");
		return;
	}

	bool found = false;
	found |= printRelocationsOf(".rel.dyn",DT_REL,relTypes,typeCount);
	found |= printRelocationsOf(".rela.dyn",DT_RELA,relTypes,typeCount);
	found |= printRelocationsOf(".rel.plt",DT_REL,relTypes,typeCount);
	found |= printRelocationsOf(".rela.plt",DT_RELA,relTypes,typeCount);
	if(!found)
		printf("There are no relocations in this file.\n\n");
}

static bool printRelocationsOf(const char *section,int type,const char **relTypes,size_t typeCount) {
	sElfSHeader *relSec = getSecByName(section);
	size_t relSize = type == DT_REL ? sizeof(sElfRel) : sizeof(sElfRela);
	size_t relCount = relSec ? relSec->sh_size / relSize : 0;
	if(relSec == NULL || relCount == 0)
		return false;

	/* relocations */
	void *rel = malloc(relCount * relSize);
	if(!rel)
		error("Not enough mem for relocations");
	readat(relSec->sh_offset,rel,relCount * relSize);
	printf("Relocation section '%s' at offset %#x contains %d entries:\n",
			section,relSec->sh_offset,relCount);
	if(type == DT_REL)
		printRelTable((sElfRel*)rel,relCount,relTypes,typeCount);
	else
		printRelaTable((sElfRela*)rel,relCount,relTypes,typeCount);
	free(rel);
	return true;
}

static void printRelTable(sElfRel *rel,size_t relCount,const char **relTypes,size_t typeCount) {
	char symcopy[MAX_SYM_LEN];
	size_t i;
	printf("%-8s %-"INFOW"s %-16s %-"SYMVALW"s %s\n","Offset","Info","Type","Sym.Val.","Sym. Name");
	for(i = 0; i < relCount; i++) {
		uint relType = ELF_R_TYPE(rel[i].r_info);
		uint symIndex = ELF_R_SYM(rel[i].r_info);
		printf("%08lx %0"INFOW"lx %-16s",(ulong)rel[i].r_offset,(ulong)rel[i].r_info,
			relType < typeCount ? relTypes[relType] : "-Unknown-");
		if(symIndex != 0) {
			sElfSym *sym = dynsyms + symIndex;
			const char *symName = dynstrtbl + sym->st_name;
			if(symName)
				strnzcpy(symcopy,symName,sizeof(symcopy));
			else
				symcopy[0] = '\0';
			printf(" %0"SYMVALW"x %-25s",sym->st_value,symcopy);
		}
		printf("\n");
	}
	printf("\n");
}

static void printRelaTable(sElfRela *rel,size_t relCount,const char **relTypes,size_t typeCount) {
	char symcopy[MAX_SYM_LEN];
	size_t i;
	printf("%-8s %-"INFOW"s %-16s %-"SYMVALW"s %s\n","Offset","Info","Type","Sym.Val.","Sym. Name + Addend");
	for(i = 0; i < relCount; i++) {
		uint relType = ELF_R_TYPE(rel[i].r_info);
		uint symIndex = ELF_R_SYM(rel[i].r_info);
		printf("%08lx %0"INFOW"lx %-16s",(ulong)rel[i].r_offset,(ulong)rel[i].r_info,
			relType < typeCount ? relTypes[relType] : "-Unknown-");
		if(symIndex != 0) {
			sElfSym *sym = dynsyms + symIndex;
			const char *symName = dynstrtbl + sym->st_name;
			if(symName)
				strnzcpy(symcopy,symName,sizeof(symcopy));
			else
				symcopy[0] = '\0';
			printf(" %0"SYMVALW"x %s + %x",sym->st_value,symcopy,rel[i].r_addend);
		}
		else
			printf(" %"SYMVALW"s %x","",rel[i].r_addend);
		printf("\n");
	}
	printf("\n");
}

static void readat(off_t offset,void *buffer,size_t count) {
	if(seek(fd,offset,SEEK_SET) < 0)
		error("Unable to seek to %x",offset);
	if(IGNSIGS(read(fd,buffer,count)) != (ssize_t)count)
		error("Unable to read %d bytes @ %x",count,offset);
}
