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

#include <esc/dir.h>
#include "path.h"

using namespace std;

Path::list_type Path::buildParts(const string &path) {
	char tmp[MAX_PATH_LEN];
	list_type parts;
	cleanpath(tmp,sizeof(tmp),path.c_str());
	parts.push_back(Link("/","/"));
	char *s,*p = tmp + 1;
	while((s = strchr(p,'/'))) {
		parts.push_back(Link(string(p,s - p),string(tmp,s - tmp)));
		p = s + 1;
	}
	return parts;
}
