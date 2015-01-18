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

#include <sys/common.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "names.h"

esc::Treap<BaseClass> PCINames::classes;
esc::Treap<Vendor> PCINames::vendors;
const char *PCINames::_file;
size_t PCINames::_fileSize;

Device *Vendor::getDevice(unsigned id) const {
	// load the devices lazy so that we don't need to store all devices in our data-structure but
	// only the devices of vendors that we actually have
	if(_devs == NULL) {
		_devs = new esc::Treap<Device>();
		const char *line = strchr(_name,'\n') + 1;
		// strchr might return NULL
		if(line != (const char*)1) {
			while(line < PCINames::_file + PCINames::_fileSize) {
				const char *lineend = strchr(line,'\n');

				// ignore comments and empty lines
				if(line[0] == '\n' || line[0] == '#') {
					line = lineend + 1;
					continue;
				}

				// device
				if(line[0] == '\t') {
					if(line[1] != '\t') {
						unsigned dev;
						const char *name = PCINames::readFromLine(line + 1,&dev);
						Device *d = new Device(name,dev);
						_devs->insert(d);
					}
				}
				else
					break;
				line = lineend + 1;
			}
		}
	}

	return _devs->find(id);
}

const char *PCINames::readFromLine(const char *line,unsigned *id) {
	char *end;
	*id = strtoul(line,&end,16);
	while(isspace(*end))
		end++;
	return end;
}

void PCINames::load(const char *file) {
	// the idea here is to mmap the file and not copy the names out of it but store the pointers
	// this way, we only need to copy the names of the devices, vendors etc. that we actually have
	// on a particular machine.
	int fd = open(file,O_RDONLY);
	if(fd < 0)
		error("open of %s failed",file);
	_fileSize = filesize(fd);
	_file = (const char*)mmap(NULL,_fileSize,_fileSize,PROT_READ,MAP_PRIVATE,fd,0);
	if(_file == NULL)
		error("mmap of %s failed",file);
	close(fd);

	unsigned id;
	BaseClass *base = NULL;
	SubClass *sub = NULL;
	bool inClasses = false;
	const char *line = _file;
	while(line < _file + _fileSize) {
		const char *lineend = strchr(line,'\n');

		// ignore comments and empty lines
		if(line[0] == '\n' || line[0] == '#') {
			line = lineend + 1;
			continue;
		}

		// class
		if(line[0] == 'C' && line[1] == ' ') {
			const char *name = readFromLine(line + 2,&id);
			base = new BaseClass(name,id);
			classes.insert(base);
			inClasses = true;
		}
		// vendor
		else if(line[0] != '\t') {
			const char *name = readFromLine(line,&id);
			Vendor *n = new Vendor(name,id);
			vendors.insert(n);
		}
		else if(inClasses) {
			// subclass
			if(line[0] == '\t' && line[1] != '\t') {
				const char *name = readFromLine(line + 1,&id);
				sub = new SubClass(name,id);
				base->subclasses.insert(sub);
			}
			// prog if
			else if(line[0] == '\t' && line[1] == '\t') {
				const char *name = readFromLine(line + 2,&id);
				ProgIF *pi = new ProgIF(name,id);
				sub->progifs.insert(pi);
			}
		}

		line = lineend + 1;
	}
}
