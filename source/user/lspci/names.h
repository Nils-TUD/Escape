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

#include <esc/col/treap.h>
#include <sys/common.h>
#include <string>

class PCINode : public esc::TreapNode<unsigned> {
public:
	explicit PCINode(const char *n,unsigned id) : esc::TreapNode<unsigned>(id), _name(n) {
	}

	std::string getName() const {
		char *end = strchr(_name,'\n');
		return std::string(_name,end - _name);
	}

protected:
	const char *_name;
};

class Device : public PCINode {
public:
	explicit Device(const char *n,unsigned id) : PCINode(n,id) {
	}
};

class Vendor : public PCINode {
public:
	static esc::Treap<Vendor> *getVendors();

	explicit Vendor(const char *n,unsigned id) : PCINode(n,id), _devs() {
	}

	Device *getDevice(unsigned id) const;

private:
	mutable esc::Treap<Device> *_devs;
};

class ProgIF : public PCINode {
public:
	explicit ProgIF(const char *n,unsigned id) : PCINode(n,id) {
	}
};

class SubClass : public PCINode {
public:
	explicit SubClass(const char *n,unsigned id) : PCINode(n,id), progifs() {
	}

	esc::Treap<ProgIF> progifs;
};

class BaseClass : public PCINode {
public:
	static esc::Treap<BaseClass> *getClasses();

	explicit BaseClass(const char *n,unsigned id) : PCINode(n,id), subclasses() {
	}

	esc::Treap<SubClass> subclasses;
};

class PCINames {
	friend class Vendor;

public:
	static void load(const char *file);

	static esc::Treap<BaseClass> classes;
	static esc::Treap<Vendor> vendors;

private:
	static const char *readFromLine(const char *line,unsigned *id);

	static const char *_file;
	static size_t _fileSize;
};
