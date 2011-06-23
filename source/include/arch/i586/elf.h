/**
 * $Id$
 */

#ifndef I586_ELF_H_
#define I586_ELF_H_

#include <esc/common.h>

typedef Elf32_Word sElfWord;
typedef Elf32_Half sElfHalf;
typedef Elf32_Off sElfOff;
typedef Elf32_Addr sElfAddr;
typedef Elf32_Section sElfSection;

typedef Elf32_Ehdr sElfEHeader;
typedef Elf32_Phdr sElfPHeader;
typedef Elf32_Shdr sElfSHeader;

typedef Elf32_Sym sElfSym;
typedef Elf32_Dyn sElfDyn;
typedef Elf32_Rel sElfRel;

#define ELF_TYPE		32

#endif /* I586_ELF_H_ */
