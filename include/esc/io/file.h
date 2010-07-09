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

#ifndef FILE_H_
#define FILE_H_

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <esc/util/vector.h>

typedef struct sFile sFile;

struct sFile {
/* private: */
	sFileInfo _info;
	char *_name;
	char *_path;

/* public: */
	/**
	 * Builds a vector with all entries in the directory denoted by this file-object.
	 * Note that you HAVE TO free the vector via vec_destroy(v,true); when you're done!
	 *
	 * @param f the file-object
	 * @param showHidden wether to include hidden files/folders
	 * @return the vector with sDirEntry* elements
	 */
	sVector *(*listFiles)(sFile *f,bool showHidden);

	/**
	 * Like listFiles(), but lets you specify a pattern.
	 *
	 * @param f the file-object
	 * @param pattern a pattern the files have to match
	 * @param showHidden wether to include hidden files/folders
	 * @return the vector with sDirEntry* elements
	 */
	sVector *(*listMatchingFiles)(sFile *f,const char *pattern,bool showHidden);

	/**
	 * @param f the file-object
	 * @return true if its a file
	 */
	bool (*isFile)(sFile *f);

	/**
	 * @param f the file-object
	 * @return true if its a directory
	 */
	bool (*isDir)(sFile *f);

	/**
	 * Returns a pointer to the name-beginning of the internal path
	 *
	 * @param f the file
	 * @return a pointer to the name
	 */
	const char *(*name)(sFile *f);

	/**
	 * Returns a pointer to the path-beginning of the internal path
	 *
	 * @param f the file
	 * @return a pointer to the path
	 */
	const char *(*parent)(sFile *f);

	/**
	 * Writes the name of this file into the given buffer
	 *
	 * @param f the file-object
	 * @param buf the buffer
	 * @param size the size of the buffer
	 * @return the length of the name
	 */
	u32 (*getName)(sFile *f,char *buf,u32 size);

	/**
	 * Writes the path of the parent-directory into the given buffer
	 *
	 * @param f the file-object
	 * @param buf the buffer
	 * @param size the size of the buffer
	 * @return the length of the parent-path
	 */
	u32 (*getParent)(sFile *f,char *buf,u32 size);

	/**
	 * Writes the absolute path of this file into the given buffer
	 *
	 * @param f the file-object
	 * @param buf the buffer
	 * @param size the size of the buffer
	 * @return the length of the path
	 */
	u32 (*getAbsolute)(sFile *f,char *buf,u32 size);

	/**
	 * Returns a pointer to the sFileInfo-struct with information about the file
	 *
	 * @param f the file-object
	 * @return the pointer
	 */
	sFileInfo *(*getInfo)(sFile *f);

	/**
	 * @param f the file-object
	 * @return the size of this file
	 */
	s32 (*getSize)(sFile *f);

	/**
	 * Destroys this file (free's all memory)
	 *
	 * @param f the file-object
	 */
	void (*destroy)(sFile *f);
};

/**
 * Creates an sFile-object for the given path. The path doesn't need to be absolute. It will
 * be made absolute with abspath(), i.e. depending on CWD.
 *
 * @param path the path
 * @return the file-object
 */
sFile *file_get(const char *path);

/**
 * Creates an sFile-object for path "<parent>/<filename>". The path doesn't need to be absolute.
 * It will be made absolute with abspath(), i.e. depending on CWD.
 *
 * @param parent the parent-path
 * @param filename the filename in <parent>
 * @return the file-object
 */
sFile *file_getIn(const char *parent,const char *filename);

#endif /* FILE_H_ */
