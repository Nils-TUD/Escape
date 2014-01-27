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

#include <esc/common.h>
#include <esc/driver/screen.h>
#include <esc/width.h>
#include <esc/messages.h>
#include <usergroup/user.h>
#include <usergroup/group.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <file.h>
#include <cmdargs.h>
#include <env.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#define DATE_LEN			(SSTRLEN("2009-09-09 14:12") + 1)

#define F_LONG_SHIFT		0
#define F_INODE_SHIFT		1
#define F_ALL_SHIFT			2
#define F_DIRSIZE_SHIFT		3
#define F_DIRNUM_SHIFT		4
#define F_NUMERIC_SHIFT		5

#define F_LONG				(1 << F_LONG_SHIFT)
#define F_INODE				(1 << F_INODE_SHIFT)
#define F_ALL				(1 << F_ALL_SHIFT)
#define F_DIRSIZE			(1 << F_DIRSIZE_SHIFT)
#define F_DIRNUM			(1 << F_DIRNUM_SHIFT)
#define F_NUMERIC			(1 << F_NUMERIC_SHIFT)

/* for calculating the widths of the fields */
#define WIDTHS_COUNT		6
#define W_INODE				0
#define W_LINKCOUNT			1
#define W_UID				2
#define W_GID				3
#define W_SIZE				4
#define W_NAME				5

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

static void printColor(const lsfile *f);
static bool compareEntries(const lsfile* a,const lsfile* b);
static vector<lsfile*> getEntries(const string& path);
static lsfile* buildFile(const file& f);
static file::size_type getDirSize(const file& d);
static void printMode(file::mode_type mode);
static void printPerm(file::mode_type mode,file::mode_type fl,char c);

static void usage(const char *name) {
	cerr << "Usage: " << name << " [-liasNn] [<path>]\n";
	cerr << "	-l: long listing\n";
	cerr << "	-i: print inode-numbers\n";
	cerr << "	-a: print also '.' and '..'\n";
	cerr << "	-s: print size of dir-content instead of directory-entries (implies -l)\n";
	cerr << "	-N: print number of directory-entries instead of their size (implies -l)\n";
	cerr << "	-n: print numeric user and group-ids\n";
	exit(EXIT_FAILURE);
}

static uint flags;

int main(int argc,char *argv[]) {
	size_t widths[WIDTHS_COUNT] = {0};
	sScreenMode mode;
	sUser *userList = nullptr;
	sGroup *groupList = nullptr;

	// parse params
	cmdargs args(argc,argv,cmdargs::MAX1_FREE);
	try {
		bool flong = 0,finode = 0,fall = 0,fdirsize,fnumeric,fdirnum;
		args.parse("l i a s n N",&flong,&finode,&fall,&fdirsize,&fnumeric,&fdirnum);
		if(args.is_help())
			usage(argv[0]);
		flags = (flong << F_LONG_SHIFT) | (finode << F_INODE_SHIFT) | (fall << F_ALL_SHIFT) |
			(fdirsize << F_DIRSIZE_SHIFT) | (fdirnum << F_DIRNUM_SHIFT) | (fnumeric << F_NUMERIC_SHIFT);
		// dirsize and dirnum imply long
		if(flags & (F_DIRSIZE | F_DIRNUM))
			flags |= F_LONG;
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	// use CWD or given path
	string path;
	if(!args.get_free().empty()) {
		path = *(args.get_free()[0]);
		env::absolutify(path);
	}
	else
		path = env::get("CWD");

	// get console-size
	if(screen_getMode(STDIN_FILENO,&mode) < 0)
		error("Unable to determine screensize");

	// read users and groups
	if(flags & F_LONG) {
		userList = user_parseFromFile(USERS_PATH,nullptr);
		if(!userList) {
			printe("Warning: unable to parse users from file");
			flags |= F_NUMERIC;
		}
		groupList = group_parseFromFile(GROUPS_PATH,nullptr);
		if(!groupList) {
			printe("Unable to parse groups from file");
			flags |= F_NUMERIC;
		}
	}

	// get entries and sort them
	vector<lsfile*> entries;
	try {
		entries = getEntries(path);
		sort(entries.begin(),entries.end(),compareEntries);
	}
	catch(const io_exception& e) {
		cerr << "Unable to read dir-entries: " << e.what() << '\n';
		exit(EXIT_FAILURE);
	}

	// calc widths
	for(auto it = entries.begin(); it != entries.end(); ++it) {
		size_t x;
		lsfile *f = *it;
		if(flags & F_INODE) {
			if((x = count_digits(f->inode(),10)) > widths[W_INODE])
				widths[W_INODE] = x;
		}
		if(flags & F_LONG) {
			if((x = count_digits(f->links(),10)) > widths[W_LINKCOUNT])
				widths[W_LINKCOUNT] = x;
			sUser *u = (~flags & F_NUMERIC) ? user_getById(userList,f->uid()) : nullptr;
			if(!u || (flags & F_NUMERIC)) {
				if((x = count_digits((ulong)f->uid(),10)) > widths[W_UID])
					widths[W_UID] = x;
			}
			else if((x = strlen(u->name)) > widths[W_UID])
				widths[W_UID] = x;
			sGroup *g = (~flags & F_NUMERIC) ? group_getById(groupList,f->gid()) : nullptr;
			if(!u || (flags & F_NUMERIC)) {
				if((x = count_digits((ulong)f->gid(),10)) > widths[W_GID])
					widths[W_GID] = x;
			}
			else if((x = strlen(g->name)) > widths[W_GID])
				widths[W_GID] = x;
			if((x = count_digits(f->rsize(),10)) > widths[W_SIZE])
				widths[W_SIZE] = x;
		}
		else {
			if((x = f->name().size()) > widths[W_NAME])
				widths[W_NAME] = x;
		}
	}

	// display
	size_t pos = 0;
	for(auto it = entries.begin(); it != entries.end(); ++it) {
		lsfile *f = *it;
		if(flags & F_LONG) {
			if(flags & F_INODE)
				cout << setw(widths[W_INODE]) << f->inode() << ' ';
			printMode(f->mode());
			cout << setw(widths[W_LINKCOUNT]) << f->links() << ' ';

			sUser *u = (~flags & F_NUMERIC) ? user_getById(userList,f->uid()) : nullptr;
			if(!u || (flags & F_NUMERIC))
				cout << setw(widths[W_UID]) << f->uid() << ' ';
			else
				cout << setw(widths[W_UID]) << (u ? u->name : "?") << ' ';
			sGroup *g = (~flags & F_NUMERIC) ? group_getById(groupList,f->gid()) : nullptr;
			if(!g || (flags & F_NUMERIC))
				cout << setw(widths[W_GID]) << f->gid() << ' ';
			else
				cout << setw(widths[W_GID]) << (g ? g->name : "?") << ' ';

			cout << setw(widths[W_SIZE]) << f->rsize() << ' ';
			{
				char dateStr[DATE_LEN];
				file::time_type ts = f->modified();
				struct tm *date = gmtime(&ts);
				strftime(dateStr,sizeof(dateStr),"%Y-%m-%d %H:%M",date);
				cout << dateStr << ' ';
			}
			printColor(f);
			cout << f->name() << "\033[co]" << '\n';
		}
		else {
			/* if the entry does not fit on the line, use next */
			if(pos + widths[W_NAME] + widths[W_INODE] + 2 >= mode.cols) {
				cout << '\n';
				pos = 0;
			}
			if(flags & F_INODE)
				cout << setw(widths[W_INODE]) << f->inode() << ' ';
			printColor(f);
			cout << setw(widths[W_NAME] + 1) << left << f->name() << "\033[co]";
			pos += widths[W_NAME] + widths[W_INODE] + 2;
		}
	}

	if(!(flags & F_LONG))
		cout << '\n';
	return EXIT_SUCCESS;
}

static void printColor(const lsfile *f) {
	if(f->is_dir())
		cout << "\033[co;9]";
	else if(S_ISCHR(f->mode()) || S_ISBLK(f->mode()) || S_ISFS(f->mode()) || S_ISSERV(f->mode()))
		cout << "\033[co;14]";
	else if(f->mode() & (S_IXUSR | S_IXGRP | S_IXOTH))
		cout << "\033[co;2]";
}

static bool compareEntries(const lsfile* a,const lsfile* b) {
	if(a->is_dir() == b->is_dir())
		return a->name() < b->name();
	if(a->is_dir())
		return true;
	return false;
}

static vector<lsfile*> getEntries(const string& path) {
	vector<lsfile*> res;
	try {
		file dir(path);
		if(dir.is_dir()) {
			vector<sDirEntry> files = dir.list_files(flags & F_ALL);
			for(auto it = files.begin(); it != files.end(); ++it) {
				try {
					res.push_back(buildFile(file(path,it->name)));
				}
				catch(const exception &e) {
					fprintf(stderr,"Skipping '%s/%s': %s\n",path.c_str(),it->name,e.what());
				}
			}
		}
		else
			res.push_back(buildFile(dir));
	}
	catch(const exception &e) {
		fprintf(stderr,"Skipping '%s': %s\n",path.c_str(),e.what());
	}
	return res;
}

static lsfile* buildFile(const file& f) {
	lsfile *lsf = new lsfile(f);
	if(f.is_dir() && (flags & (F_DIRSIZE | F_DIRNUM))) {
		if(flags & F_DIRNUM) {
			vector<sDirEntry> files = lsf->list_files(flags & F_ALL);
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
	vector<sDirEntry> files = d.list_files((flags & F_ALL) != 0);
	for(auto it = files.begin(); it != files.end(); ++it) {
		file f(path,it->name);
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

static void printMode(file::mode_type mode) {
	if(S_ISDIR(mode))
		cout << 'd';
	else if(S_ISCHR(mode))
		cout << 'c';
	else if(S_ISBLK(mode))
		cout << 'b';
	else if(S_ISFS(mode))
		cout << 'f';
	else if(S_ISSERV(mode))
		cout << 's';
	else
		cout << '-';
	printPerm(mode,S_IRUSR,'r');
	printPerm(mode,S_IWUSR,'w');
	printPerm(mode,S_IXUSR,'x');
	printPerm(mode,S_IRGRP,'r');
	printPerm(mode,S_IWGRP,'w');
	printPerm(mode,S_IXGRP,'x');
	printPerm(mode,S_IROTH,'r');
	printPerm(mode,S_IWOTH,'w');
	printPerm(mode,S_IXOTH,'x');
	cout << ' ';
}

static void printPerm(file::mode_type mode,file::mode_type fl,char c) {
	if((mode & fl) != 0)
		cout << c;
	else
		cout << '-';
}
