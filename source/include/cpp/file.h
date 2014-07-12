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

#include <esc/io.h>
#include <sys/stat.h>
#include <vthrow.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <time.h>

namespace std {
	class file {
	public:
		typedef size_t size_type;
		typedef time_t time_type;
		typedef ushort user_type;
		typedef ushort group_type;
		typedef ino_t ino_type;
		typedef dev_t dev_type;
		typedef ushort mode_type;

	public:
		/**
		 * Builds a file-object for given path
		 *
		 * @param path the path (has not to be absolute)
		 * @throws default_error if stat fails
		 */
		file(const string& path);
		/**
		 * Builds a file-object for <name> in <parent>
		 *
		 * @param parent the parent-path (has not to be absolute)
		 * @param name the filename
		 * @throws default_error if stat fails
		 */
		file(const string& parent,const string& name);
		/**
		 * Copy-constructor
		 */
		file(const file& f);
		/**
		 * Assignment-op
		 */
		file& operator =(const file& f);
		/**
		 * Destructor
		 */
		virtual ~file();

		/**
		 * Builds a vector with all entries in the directory denoted by this file-object.
		 *
		 * @param showHidden whether to include hidden files/folders
		 * @param pattern a pattern the files have to match
		 * @return the vector
		 */
		vector<struct dirent> list_files(bool showHidden,const string& pattern = string()) const;

		/**
		 * @return the mode of the file
		 */
		mode_type mode() const {
			return _info.st_mode;
		}

		/**
		 * @return whether its a file
		 */
		bool is_file() const {
			return S_ISREG(_info.st_mode);
		}
		/**
		 * @return whether its a directory
		 */
		bool is_dir() const {
			return S_ISDIR(_info.st_mode);
		}
		/**
		 * @return the size of the file in bytes
		 */
		size_type size() const {
			return _info.st_size;
		}

		/**
		 * @return the inode-number
		 */
		ino_type inode() const {
			return _info.st_ino;
		}
		/**
		 * @return the device-number
		 */
		dev_type device() const {
			return _info.st_dev;
		}

		/**
		 * @return the user-id
		 */
		user_type uid() const {
			return _info.st_uid;
		}
		/**
		 * @return the group-id
		 */
		group_type gid() const {
			return _info.st_gid;
		}
		/**
		 * @return the number of hardlinks to it
		 */
		size_type links() const {
			return _info.st_nlink;
		}

		/**
		 * @return timestamp of last modification
		 */
		time_type modified() const {
			return _info.st_mtime;
		}
		/**
		 * @return timestamp of last access
		 */
		time_type accessed() const {
			return _info.st_atime;
		}
		/**
		 * @return timestamp of creation
		 */
		time_type created() const {
			return _info.st_ctime;
		}

		/**
		 * @return the filename
		 */
		const string &name() const {
			return _name;
		}
		/**
		 * @return the parent-path
		 */
		const string &parent() const {
			return _parent;
		}
		/**
		 * @return the absolute path
		 */
		string path() const {
			return _parent + "/" + _name;
		}

	private:
		/**
		 * Inits _info and _path
		 *
		 * @param parent the parent-path
		 * @param name the filename
		 * @throws default_error if stat fails
		 */
		void init(const string& parent,const string& name);

	private:
		struct stat _info;
		string _parent;
		string _name;
	};
}
