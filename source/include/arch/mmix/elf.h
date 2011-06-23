/**
 * $Id$
 */

#ifndef MMIX_ELF_H_
#define MMIX_ELF_H_

#include <esc/common.h>

typedef Elf64_Word sElfWord;
typedef Elf64_Half sElfHalf;
typedef Elf64_Off sElfOff;
typedef Elf64_Addr sElfAddr;
typedef Elf64_Section sElfSection;

typedef Elf64_Ehdr sElfEHeader;
typedef Elf64_Phdr sElfPHeader;
typedef Elf64_Shdr sElfSHeader;

typedef Elf64_Sym sElfSym;
typedef Elf64_Dyn sElfDyn;
typedef Elf64_Rel sElfRel;

#define ELF_TYPE		64

#endif /* MMIX_ELF_H_ */
