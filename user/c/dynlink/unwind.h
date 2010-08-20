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

#ifndef UNWIND_H_
#define UNWIND_H_

#include <stddef.h>

struct dwarf_cie
{
  u32 length;
  s32 CIE_id;
  u8 version;
  unsigned char augmentation[];
} __attribute__ ((packed, aligned (__alignof__ (void *))));

/* The first few fields of an FDE.  */
struct dwarf_fde
{
  u32 length;
  s32 CIE_delta;
  unsigned char pc_begin[];
} __attribute__ ((packed, aligned (__alignof__ (void *))));
typedef struct dwarf_fde fde;

/* borrowed from unwind-dw2-fde.h of gcc (changed a bit) */
/* the content of this is not important for us; just the size */
struct object
{
  void *pc_begin;
  void *tbase;
  void *dbase;
  union {
    const struct dwarf_fde *single;
    struct dwarf_fde **array;
  } u;

  union {
    struct {
      unsigned long sorted : 1;
      unsigned long from_array : 1;
      unsigned long mixed_encoding : 1;
      unsigned long encoding : 8;
      /* ??? Wish there was an easy way to detect a 64-bit host here;
	 we've got 32 bits left to play with...  */
      unsigned long count : 21;
    } b;
    size_t i;
  } s;

  struct object *next;
};

typedef void (*fRegFrameInfoBases)(const void *begin, struct object *ob,void *tbase, void *dbase);

#endif /* UNWIND_H_ */
