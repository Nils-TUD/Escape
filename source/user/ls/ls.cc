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

#include <esc/proto/vterm.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/env.h>
#include <esc/file.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <usergroup/usergroup.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

using namespace std;
using namespace esc;

enum {
	F_LONG_SHIFT,
	F_INODE_SHIFT,
	F_ALL_SHIFT,
	F_DIRSIZE_SHIFT,
	F_DIRNUM_SHIFT,
	F_NUMERIC_SHIFT,
	F_HUMAN_SHIFT,
};

enum {
	F_LONG				= 1 << F_LONG_SHIFT,
	F_INODE				= 1 << F_INODE_SHIFT,
	F_ALL				= 1 << F_ALL_SHIFT,
	F_DIRSIZE			= 1 << F_DIRSIZE_SHIFT,
	F_DIRNUM			= 1 << F_DIRNUM_SHIFT,
	F_NUMERIC			= 1 << F_NUMERIC_SHIFT,
	F_HUMAN				= 1 << F_HUMAN_SHIFT,
};

/* for calculating the widths of the fields */
enum {
	W_INODE,
	W_LINKCOUNT,
	W_UID,
	W_GID,
	W_SIZE,
	W_NAME,
	WIDTHS_COUNT,
};

class lsfile : public file {
public:
	lsfile(const string& p)
		: file(p), _rsize(0) {
	}
	lsfile(const lsfile& f)
		: file(f), _rsize(f._rsize) {
	}
	lsfile(const file& f)
		: file(f), _rsize(0) {
	}
	lsfile& operator =(const lsfile& f) {
		file::operator =(f);
		_rsize = f._rsize;
		return *this;
	}
	virtual ~lsfile() {
	}

	size_type rsize() const {
		return _rsize;
	}
	void rsize(size_type s) {
		_rsize = s;
	}

private:
	size_type _rsize;
};

struct DirList {
	bool dir;
	std::string path;
	std::vector<lsfile*> entries;
};

static DirList collectEntries(const char *path,size_t *widths,bool showPath);
static void printDir(const std::string &path,const std::vector<lsfile*> &entries,size_t *widths,
		uint cols,bool showPath);
static void printColor(const lsfile *f);
static bool compareEntries(const lsfile* a,const lsfile* b);
static vector<lsfile*> getEntries(const string& path);
static lsfile* buildFile(const file& f);
static file::size_type getDirSize(const file& d);
static void printSize(size_t size,size_t width);
static void printMode(file::mode_type mode);
static void printPerm(file::mode_type mode,file::mode_type fl,char c);

static void usage(const char *name) {
	serr << "Usage: " << name << " [-liasNn] [<path>...]\n";
	serr << "    -l: long listing\n";
	serr << "    -h: print human readable sizes\n";
	serr << "    -i: print inode-numbers\n";
	serr << "    -a: print also '.' and '..'\n";
	serr << "    -s: print size of dir-content instead of directory-entries (implies -l)\n";
	serr << "    -N: print number of directory-entries instead of their size (implies -l)\n";
	serr << "    -n: print numeric user and group-ids\n";
	exit(EXIT_FAILURE);
}

static uint flags;
static sNamedItem *userList;
static sNamedItem *groupList;

int main(int argc,char *argv[]) {
	// parse params
	cmdargs args(argc,argv,0);
	try {
		int flong = 0,fhuman = 0, finode = 0,fall = 0,fdirsize = 0,fnumeric = 0,fdirnum = 0;
		args.parse("l h i a s n N",&flong,&fhuman,&finode,&fall,&fdirsize,&fnumeric,&fdirnum);
		if(args.is_help())
			usage(argv[0]);
		flags = (flong << F_LONG_SHIFT) | (fhuman << F_HUMAN_SHIFT) |
			(finode << F_INODE_SHIFT) | (fall << F_ALL_SHIFT) | (fdirsize << F_DIRSIZE_SHIFT) |
			(fdirnum << F_DIRNUM_SHIFT) | (fnumeric << F_NUMERIC_SHIFT);
		// dirsize and dirnum imply long
		if(flags & (F_DIRSIZE | F_DIRNUM))
			flags |= F_LONG;
	}
	catch(const cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	// get console-size
	uint cols = esc::VTerm::getSize(esc::env::get("TERM").c_str()).first;

	// read users and groups
	if(flags & F_LONG) {
		userList = usergroup_parse(USERS_PATH,nullptr);
		if(!userList) {
			errmsg("Warning: unable to parse users from file");
			flags |= F_NUMERIC;
		}
		groupList = usergroup_parse(GROUPS_PATH,nullptr);
		if(!groupList) {
			errmsg("Unable to parse groups from file");
			flags |= F_NUMERIC;
		}
	}

	// first, collect all files and count widths
	size_t widths[WIDTHS_COUNT] = {0};
	std::vector<DirList> lists;
	if(!args.get_free().empty()) {
		size_t count = args.get_free().size();
		for(auto &p : args.get_free())
			lists.push_back(collectEntries(p->c_str(),widths,count > 1));
	}
	else {
		std::string path = env::get("CWD");
		lists.push_back(collectEntries(path.c_str(),widths,false));
	}

	// now print them
	size_t i = 0,count = lists.size();
	bool lastDir = false;
	for(auto &l : lists) {
		if(i > 0 && (l.dir || lastDir))
			sout << '\n';
		if(count > 1 && l.dir)
			sout << l.path << ":\n";

		printDir(l.path,l.entries,widths,cols,count > 1);
		lastDir = l.dir;
		i++;
	}
	return EXIT_SUCCESS;
}

static DirList collectEntries(const char *path,size_t *widths,bool showPath) {
	DirList list;
	list.dir = isdir(path);
	if(showPath) {
		if(!list.dir) {
			char tmp[MAX_PATH_LEN];
			strnzcpy(tmp,path,sizeof(tmp));
			list.path = dirname(tmp);
		}
		else
			list.path = path;
		if(list.path[list.path.length() - 1] != '/')
			list.path += '/';
	}

	// get entries and sort them
	try {
		list.entries = getEntries(path);
		sort(list.entries.begin(),list.entries.end(),compareEntries);
	}
	catch(const default_error& e) {
		errmsg("Unable to read dir-entries: " << e.what());
	}

	// calc widths
	for(auto it = list.entries.begin(); it != list.entries.end(); ++it) {
		size_t x;
		lsfile *f = *it;
		if(flags & F_INODE) {
			if((x = count_digits(f->inode(),10)) > widths[W_INODE])
				widths[W_INODE] = x;
		}
		if(flags & F_LONG) {
			if((x = count_digits(f->links(),10)) > widths[W_LINKCOUNT])
				widths[W_LINKCOUNT] = x;
			sNamedItem *u = (~flags & F_NUMERIC) ? usergroup_getById(userList,f->uid()) : nullptr;
			if(!u || (flags & F_NUMERIC)) {
				if((x = count_digits((ulong)f->uid(),10)) > widths[W_UID])
					widths[W_UID] = x;
			}
			else if((x = strlen(u->name)) > widths[W_UID])
				widths[W_UID] = x;
			sNamedItem *g = (~flags & F_NUMERIC) ? usergroup_getById(groupList,f->gid()) : nullptr;
			if(!u || (flags & F_NUMERIC)) {
				if((x = count_digits((ulong)f->gid(),10)) > widths[W_GID])
					widths[W_GID] = x;
			}
			else if((x = strlen(g->name)) > widths[W_GID])
				widths[W_GID] = x;
			// human readable is at most 3 digits plus binary prefix
			if(flags & F_HUMAN)
				widths[W_SIZE] = 4;
			else if((x = count_digits(f->rsize(),10)) > widths[W_SIZE])
				widths[W_SIZE] = x;
		}
		else {
			if((x = f->name().size() + list.path.length()) > widths[W_NAME])
				widths[W_NAME] = x;
		}
	}
	return list;
}

static void printDir(const std::string &path,const std::vector<lsfile*> &entries,size_t *widths,
		uint cols,bool showPath) {
	// display
	size_t pos = 0;
	for(auto it = entries.begin(); it != entries.end(); ++it) {
		lsfile *f = *it;
		if(flags & F_LONG) {
			if(flags & F_INODE)
				sout << fmt(f->inode(),widths[W_INODE]) << ' ';
			printMode(f->mode());
			sout << fmt(f->links(),widths[W_LINKCOUNT]) << ' ';

			sNamedItem *u = (~flags & F_NUMERIC) ? usergroup_getById(userList,f->uid()) : nullptr;
			if(!u || (flags & F_NUMERIC))
				sout << fmt(f->uid(),widths[W_UID]) << ' ';
			else
				sout << fmt((u ? u->name : "?"),widths[W_UID]) << ' ';
			sNamedItem *g = (~flags & F_NUMERIC) ? usergroup_getById(groupList,f->gid()) : nullptr;
			if(!g || (flags & F_NUMERIC))
				sout << fmt(f->gid(),widths[W_GID]) << ' ';
			else
				sout << fmt((g ? g->name : "?"),widths[W_GID]) << ' ';

			printSize(f->rsize(),widths[W_SIZE]);
			sout << ' ';
			{
				char dateStr[SSTRLEN("2009-09-09 14:12") + 1];
				file::time_type ts = f->modified();
				struct tm *date = gmtime(&ts);
				strftime(dateStr,sizeof(dateStr),"%Y-%m-%d %H:%M",date);
				sout << dateStr << ' ';
			}
			printColor(f);
			if(showPath)
				sout << path;
			sout << f->name() << "\033[co]" << '\n';
		}
		else {
			/* if the entry does not fit on the line, use next */
			if(pos + widths[W_NAME] + widths[W_INODE] + 2 >= cols) {
				sout << '\n';
				pos = 0;
			}
			if(flags & F_INODE)
				sout << fmt(f->inode(),widths[W_INODE]) << ' ';
			printColor(f);
			sout << fmt(showPath? (path + f->name()) : f->name(),"-",widths[W_NAME] + 1) << "\033[co]";
			pos += widths[W_NAME] + widths[W_INODE] + 2;
		}

		if(sout.bad())
			error("Write failed");
	}

	if(!(flags & F_LONG))
		sout << '\n';
}

static void printColor(const lsfile *f) {
	if(f->is_dir())
		sout << "\033[co;9]";
	else if(S_ISCHR(f->mode()) || S_ISBLK(f->mode()) || S_ISFS(f->mode()) || S_ISSERV(f->mode()))
		sout << "\033[co;14]";
	else if(f->mode() & (S_IXUSR | S_IXGRP | S_IXOTH))
		sout << "\033[co;2]";
}

static bool compareEntries(const lsfile* a,const lsfile* b) {
	if(a->is_dir() == b->is_dir())
		return strcasecmp(a->name().c_str(),b->name().c_str()) < 0;
	if(a->is_dir())
		return true;
	return false;
}

static vector<lsfile*> getEntries(const string& path) {
	vector<lsfile*> res;
	try {
		file dir(path);
		if(dir.is_dir()) {
			vector<struct dirent> files = dir.list_files(flags & F_ALL);
			for(auto it = files.begin(); it != files.end(); ++it) {
				try {
					res.push_back(buildFile(file(path,it->d_name)));
				}
				catch(const exception &e) {
					printe("Skipping '%s/%s': %s",path.c_str(),it->d_name,e.what());
				}
			}
		}
		else
			res.push_back(buildFile(dir));
	}
	catch(const exception &e) {
		printe("Skipping '%s': %s",path.c_str(),e.what());
	}
	return res;
}

static lsfile* buildFile(const file& f) {
	lsfile *lsf = new lsfile(f);
	if(f.is_dir() && (flags & (F_DIRSIZE | F_DIRNUM))) {
		if(flags & F_DIRNUM) {
			vector<struct dirent> files = lsf->list_files(flags & F_ALL);
			lsf->rsize(files.size());
		}
		else
			lsf->rsize(getDirSize(f));
	}
	else
		lsf->rsize(lsf->size());
	return lsf;
}

static file::size_type getDirSize(const file& d) {
	file::size_type res = 0;
	string path = d.path();
	vector<struct dirent> files = d.list_files((flags & F_ALL) != 0);
	for(auto it = files.begin(); it != files.end(); ++it) {
		file f(path,it->d_name);
		if(f.is_dir()) {
			string name = f.name();
			if(name != "." && name != "..")
				res += getDirSize(f);
		}
		else
			res += f.size();
	}
	return res;
}

static void printSize(size_t size,size_t width) {
	if(flags & F_HUMAN) {
		static const char sizes[] = {'B','K','M','G','T','P'};
		size_t sz = 1;
		for(size_t i = 0; i < ARRAY_SIZE(sizes); ++i) {
			if(size < sz * 1024) {
				sout << fmt(size / sz,width) << sizes[i];
				break;
			}
			sz *= 1024;
		}
	}
	else
		sout << fmt(size,width);
}

static void printMode(file::mode_type mode) {
	char exec = 'x';
	if(S_ISCHR(mode) || S_ISBLK(mode) || S_ISFS(mode) || S_ISSERV(mode))
		exec = 'm';
	else if(S_ISIRQ(mode))
		exec = 'i';

	if(S_ISDIR(mode))
		sout << 'd';
	else if(S_ISCHR(mode))
		sout << 'c';
	else if(S_ISBLK(mode))
		sout << 'b';
	else if(S_ISFS(mode))
		sout << 'f';
	else if(S_ISSERV(mode))
		sout << 's';
	else if(S_ISMS(mode))
		sout << 'm';
	else if(S_ISIRQ(mode))
		sout << 'i';
	else
		sout << '-';

	printPerm(mode,S_IRUSR,'r');
	printPerm(mode,S_IWUSR,'w');
	printPerm(mode,S_IXUSR,exec);
	printPerm(mode,S_IRGRP,'r');
	printPerm(mode,S_IWGRP,'w');
	printPerm(mode,S_IXGRP,exec);
	printPerm(mode,S_IROTH,'r');
	printPerm(mode,S_IWOTH,'w');
	if(S_ISDIR(mode) && (mode & S_ISSTICKY))
		sout << ((mode & S_IXOTH) ? 't' : 'T');
	else
		printPerm(mode,S_IXOTH,exec);
	sout << ' ';
}

static void printPerm(file::mode_type mode,file::mode_type fl,char c) {
	if((mode & fl) != 0)
		sout << c;
	else
		sout << '-';
}
