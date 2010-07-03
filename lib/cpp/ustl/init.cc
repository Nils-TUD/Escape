/**
 * $Id: init.cc 652 2010-05-07 10:58:50Z nasmussen $
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
#include <stdlib.h>
#include <string.h>

extern "C" {
	/**
	 * From osdev-wiki: There's also a strange function dl_iterate_phdrs. You don't need this so let
	 * it simply return -1. It's usually used to find exception frames for dynamically linked
	 * objects. You could also remove calls to this function from the library.
	 */
	int dl_iterate_phdr(void);

	/**
	 * Demangles the given name
	 *
	 * @param mangled_name A NULL-terminated character string containing the name to be demangled.
	 * @param output_buffer A region of memory, allocated with malloc, of *length bytes, into
	 * 	which the demangled name is stored. If output_buffer is not long enough, it is expanded
	 * 	using realloc. output_buffer may instead be NULL; in that case, the demangled name is
	 * 	placed in a region of memory allocated with malloc.
	 * @param length If length is non-NULL, the length of the buffer containing the demangled name
	 * 	is placed in *length.
	 * @param status *status is set to one of the following values:
	 * 		0: The demangling operation succeeded.
	 * 		-1: A memory allocation failiure occurred.
	 * 		-2: mangled_name is not a valid name under the C++ ABI mangling rules.
	 * 		-3: One of the arguments is invalid.
	 * @return A pointer to the start of the NUL-terminated demangled name, or NULL if the
	 * 	demangling fails. The caller is responsible for deallocating this memory using free.
	 */
	char* __cxa_demangle(const char *mangled_name,char *output_buffer,size_t *length,int *status);
}

/* gcc needs the symbol __dso_handle */
void *__dso_handle;

int dl_iterate_phdr(void) {
	return -1;
}

char* __cxa_demangle(const char *mangled_name,char *output_buffer,size_t *length,int *status) {
	u32 len = strlen(mangled_name);
	if(!output_buffer || (length && *length < len + 1)) {
		output_buffer = (char*)realloc(output_buffer,len + 1);
		if(!output_buffer) {
			*status = -1;
			return NULL;
		}
	}
	if(length)
		*length = len + 1;
	/* TODO for now its enough to keep the mangled name */
	strcpy(output_buffer,mangled_name);
	*status = 0;
	return output_buffer;
}
