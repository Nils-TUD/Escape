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

#include <sys/common.h>

typedef Elf32_Word ElfWord;
typedef Elf32_Half ElfHalf;
typedef Elf32_Off ElfOff;
typedef Elf32_Addr ElfAddr;
typedef Elf32_Section ElfSection;
typedef Elf32_Sword ElfSword;

typedef Elf32_Sword ElfDynTag;
typedef Elf32_Word ElfDynVal;

typedef Elf32_Ehdr sElfEHeader;
typedef Elf32_Phdr sElfPHeader;
typedef Elf32_Shdr sElfSHeader;

typedef Elf32_Sym sElfSym;
typedef Elf32_Dyn sElfDyn;
typedef Elf32_Rel sElfRel;
typedef Elf32_Rela sElfRela;

#define ELF_TYPE		32

#define ELF_R_SYM(val)	ELF32_R_SYM(val)
#define ELF_R_TYPE(val)	ELF32_R_TYPE(val)

#define R_NONE			R_386_NONE
#define R_GLOB_DAT		R_386_GLOB_DAT
#define R_COPY			R_386_COPY
#define R_RELATIVE		R_386_RELATIVE
#define R_32			R_386_32
#define R_64			0
#define R_PC32			R_386_PC32
#define R_JUMP_SLOT		R_386_JMP_SLOT
