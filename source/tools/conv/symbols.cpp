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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cxxabi.h>
#include <string>
#include "symbols.h"
#include "elf.h"

typedef struct sSymbolFile {
	Elf32_Sym *symtab;
	size_t symcount;
	char *strtab;
	struct sSymbolFile *next;
} sSymbolFile;

static void demangle(char *dst,size_t dstSize,const char *name);
static char *sym_loadShSyms(FILE *f,const Elf32_Ehdr *eheader);
static Elf32_Shdr *sym_getSecByName(FILE *f,const Elf32_Ehdr *eheader,char *syms,const char *name);
static void readat(FILE *f,off_t offset,void *buffer,size_t count);

static sSymbolFile *files;
static sSymbolFile *lastFile;

void sym_init(void) {
	files = NULL;
	lastFile = NULL;
}

void sym_addFile(const char *file) {
	Elf32_Ehdr eheader;
	Elf32_Shdr *sheader;
	char *shsyms;
	sSymbolFile *sf = (sSymbolFile*)malloc(sizeof(sSymbolFile));
	FILE *f = fopen(file,"r");
	if(!f)
		perror("fopen");
	readat(f,0,&eheader,sizeof(Elf32_Ehdr));
	shsyms = sym_loadShSyms(f,&eheader);

	sf->symtab = NULL;
	sf->symcount = 0;
	sheader = sym_getSecByName(f,&eheader,shsyms,".symtab");
	if(sheader) {
		sf->symtab = (Elf32_Sym*)malloc(sheader->sh_size);
		if(!sf->symtab)
			perror("malloc");
		readat(f,sheader->sh_offset,sf->symtab,sheader->sh_size);
		sf->symcount = sheader->sh_size / sizeof(Elf32_Sym);
	}

	sf->strtab = NULL;
	sheader = sym_getSecByName(f,&eheader,shsyms,".strtab");
	if(sheader) {
		sf->strtab = (char*)malloc(sheader->sh_size);
		if(!sf->strtab)
			perror("malloc");
		readat(f,sheader->sh_offset,sf->strtab,sheader->sh_size);
	}
	fclose(f);

	if(lastFile)
		lastFile->next = sf;
	else
		files = sf;
	sf->next = NULL;
	lastFile = sf;
}

const char *sym_resolve(unsigned long addr) {
	static char symbolName[MAX_FUNC_LEN];
	static char cleanSymbolName[MAX_FUNC_LEN];
	sSymbolFile *file = files;
	while(file != NULL) {
		for(size_t i = 0; i < file->symcount; i++) {
			if(ELF32_ST_TYPE(file->symtab[i].st_info) == STT_FUNC && file->symtab[i].st_value == addr) {
				demangle(symbolName,MAX_FUNC_LEN,file->strtab + file->symtab[i].st_name);
				return symbolName;
			}
		}
		file = file->next;
	}
	sprintf(symbolName,"%lu",addr);
	specialChars(symbolName,cleanSymbolName,sizeof(cleanSymbolName));
	return cleanSymbolName;
}

void specialChars(const char *src,char *dst,size_t dstSize) {
	std::string repl(src);
	size_t index = std::string::npos;
	while((index = repl.find("&",index + 1)) != std::string::npos)
		repl.replace(index,1,"&amp;");
	while((index = repl.find("<")) != std::string::npos)
		repl.replace(index,1,"&lt;");
	while((index = repl.find(">")) != std::string::npos)
		repl.replace(index,1,"&gt;");
	strncpy(dst,repl.c_str(),dstSize);
	dst[dstSize - 1] = '\0';
}

static void demangle(char *dst,size_t dstSize,const char *name) {
	size_t tmpSize = 80;
	char *tmp = (char*)malloc(tmpSize);
	int status;
	tmp = abi::__cxa_demangle(name,tmp,&tmpSize,&status);
	if(status == 0 && tmp)
		specialChars(tmp,dst,dstSize);
	else
		specialChars(name,dst,dstSize);
	free(tmp);
}

static char *sym_loadShSyms(FILE *f,const Elf32_Ehdr *eheader) {
	Elf32_Shdr sheader;
	unsigned char *datPtr;
	char *shsymbols;
	datPtr = (unsigned char*)(eheader->e_shoff + eheader->e_shstrndx * eheader->e_shentsize);
	readat(f,(off_t)datPtr,&sheader,sizeof(Elf32_Shdr));
	shsymbols = (char*)malloc(sheader.sh_size);
	if(shsymbols == NULL)
		perror("malloc");
	readat(f,sheader.sh_offset,shsymbols,sheader.sh_size);
	return shsymbols;
}

static Elf32_Shdr *sym_getSecByName(FILE *f,const Elf32_Ehdr *eheader,char *syms,const char *name) {
	static Elf32_Shdr section[1];
	int i;
	unsigned char *datPtr = (unsigned char*)eheader->e_shoff;
	for(i = 0; i < eheader->e_shnum; datPtr += eheader->e_shentsize, i++) {
		readat(f,(off_t)datPtr,section,sizeof(Elf32_Shdr));
		if(strcmp(syms + section->sh_name,name) == 0)
			return section;
	}
	return NULL;
}

static void readat(FILE *f,off_t offset,void *buffer,size_t count) {
	if(fseek(f,offset,SEEK_SET) < 0)
		perror("fseek");
	if(fread(buffer,1,count,f) != count)
		perror("fread");
}
