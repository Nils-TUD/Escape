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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cxxabi.h>
#include <string>
#include "symbols.h"
#include "elf.h"

static void demangle(char *dst,size_t dstSize,const char *name);
static void sym_loadShSyms(void);
static Elf32_Shdr *sym_getSecByName(const char *name);
static void readat(off_t offset,void *buffer,size_t count);

static FILE *f;
static char *shsymbols;
static Elf32_Sym *symtab;
static unsigned long symcount;
static char *strtab;
static Elf32_Ehdr eheader;

void sym_init(const char *file) {
	Elf32_Shdr *sheader;
	f = fopen(file,"r");
	if(!f)
		perror("fopen");
	readat(0,&eheader,sizeof(Elf32_Ehdr));
	sym_loadShSyms();

	sheader = sym_getSecByName(".symtab");
	if(sheader) {
		symtab = (Elf32_Sym*)malloc(sheader->sh_size);
		if(!symtab)
			perror("malloc");
		readat(sheader->sh_offset,symtab,sheader->sh_size);
		symcount = sheader->sh_size / sizeof(Elf32_Sym);
	}
	sheader = sym_getSecByName(".strtab");
	if(sheader) {
		strtab = (char*)malloc(sheader->sh_size);
		if(!strtab)
			perror("malloc");
		readat(sheader->sh_offset,strtab,sheader->sh_size);
	}
}

const char *sym_resolve(unsigned long addr) {
	static char symbolName[MAX_FUNC_LEN];
	static char cleanSymbolName[MAX_FUNC_LEN];
	for(int i = 0; i < symcount; i++) {
		if(ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC && symtab[i].st_value == addr) {
			demangle(symbolName,MAX_FUNC_LEN,strtab + symtab[i].st_name);
			return symbolName;
		}
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
	char *tmp = (char*)malloc(dstSize);
	int status;
	tmp = abi::__cxa_demangle(name,tmp,&dstSize,&status);
	if(status == 0 && tmp)
		specialChars(tmp,dst,dstSize);
	else
		specialChars(name,dst,dstSize);
	free(tmp);
}

static void sym_loadShSyms(void) {
	Elf32_Shdr sheader;
	unsigned char *datPtr;
	datPtr = (unsigned char*)(eheader.e_shoff + eheader.e_shstrndx * eheader.e_shentsize);
	readat((off_t)datPtr,&sheader,sizeof(Elf32_Shdr));
	shsymbols = (char*)malloc(sheader.sh_size);
	if(shsymbols == NULL)
		perror("malloc");
	readat(sheader.sh_offset,shsymbols,sheader.sh_size);
}

static Elf32_Shdr *sym_getSecByName(const char *name) {
	static Elf32_Shdr section[1];
	int i;
	unsigned char *datPtr = (unsigned char*)eheader.e_shoff;
	for(i = 0; i < eheader.e_shnum; datPtr += eheader.e_shentsize, i++) {
		readat((off_t)datPtr,section,sizeof(Elf32_Shdr));
		if(strcmp(shsymbols + section->sh_name,name) == 0)
			return section;
	}
	return NULL;
}

static void readat(off_t offset,void *buffer,size_t count) {
	if(fseek(f,offset,SEEK_SET) < 0)
		perror("fseek");
	if(fread(buffer,1,count,f) != count)
		perror("fread");
}
