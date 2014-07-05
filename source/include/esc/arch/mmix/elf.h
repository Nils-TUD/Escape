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

#include <esc/common.h>

typedef Elf64_Word ElfWord;
typedef Elf64_Half ElfHalf;
typedef Elf64_Off ElfOff;
typedef Elf64_Addr ElfAddr;
typedef Elf64_Section ElfSection;
typedef Elf64_Sword ElfSword;

typedef Elf64_Sxword ElfDynTag;
typedef Elf64_Xword ElfDynVal;

typedef Elf64_Ehdr sElfEHeader;
typedef Elf64_Phdr sElfPHeader;
typedef Elf64_Shdr sElfSHeader;

typedef Elf64_Sym sElfSym;
typedef Elf64_Dyn sElfDyn;
typedef Elf64_Rel sElfRel;
typedef Elf64_Rela sElfRela;

#define ELF_TYPE		64

#define ELF_R_SYM(val)	ELF64_R_SYM(val)
#define ELF_R_TYPE(val)	ELF64_R_TYPE(val)
