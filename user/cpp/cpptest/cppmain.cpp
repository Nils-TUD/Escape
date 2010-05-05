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

#include <ustl.h>

using namespace ustl;

class my {
private:
	u32 abc;
public:
	my() : abc(4) {
		printf("Constructor for %p...\n",this);
		fflush(stdout);
	};
	~my() {
		printf("Destructor for %x...\n",this);
		fflush(stdout);
		abc = 0;
	};

	void doIt();
};

my myobj;

void my::doIt() {
	printf("Ich bins: %d\n",abc);
}

int main(void) {
#if 0
	/*startThread(threadFunc);
	startThread(threadFunc);*/

	char str[10];
	char buffer[1024];
	const char *test = "ein test";
	StringStream s(str,sizeof(str));
	FileStream f("/file.txt",FileStream::READ);
	s << 'a' << 'b';
	s.write(test);
	printf("%s\n",str);
	/*char c;
	esc::in >> c;
	esc::out << c;*/
	f.read(buffer,sizeof(buffer));
	out << buffer << endl;

	out << "test" << endl;
	out << -1234 << endl;
	out << 'a' << 'b' << 'c' << endl;
	out.format("das=%d, bin=%x, ich=%s\n",-193,0xABC,"test");
	out.format("out.pos=%d\n",out.getPos());

	String mystr = "ich";
	mystr += "test";
	mystr += 'a';
	mystr += String("abc");
	String abc("test");
	out << "str=" << mystr << ", length=" << mystr.length() << endl;
	out << abc << endl;

	out << "offset of a=" << mystr.find('a') << endl;
	out << "offset of test=" << mystr.find("test") << endl;

	Vector<s32> v;
	for(s32 i = 0; i < 20; i++)
		v.add(i);
	Vector<s32> v2 = v;
	v.insert(4,1024);
	v.insert(0,123);
	v.insert(v.size(),0);
	v.insert(v.size() - 1,456);

	for(u32 i = 0; i < v.size(); i++) {
		out << i << ": " << v[i] << endl;
	}
	for(u32 i = 0; i < v2.size(); i++) {
		out << i << ": " << v2[i] << endl;
	}
#else
    vector<int> v;
    v.resize (30);
    for (size_t i = 0; i < v.size(); ++ i)
	v[i] = i;
    v.push_back (57);
    v.insert (v.begin() + 20, 555);
    v.erase (v.begin() + 3);

	/*
	String s1 = "abc";
	String s2 = "def";
	String s3 = "ghi";
	String s4("testtest");
	s1.erase(1);
	s2.erase(0);
	s3.erase(1,1);
	s4.erase(4,3);
	s1.insert(0,s2);
	s1.insert(1,'a');
	s1.insert(4,"blub");
	s2.insert(3,"vier");
	s3.insert(5,'b');
	s3.insert(2,"hier");
	out << s1 << endl;
	out << s2 << endl;
	out << s3 << endl;
	out << s4 << endl;*/
#endif
	/*unsigned int a = 0;
	a++;
	printf("a=%d\n",a);

	x.doIt();
	y->doIt();

	my *m = new my();
	m->doIt();
	delete m;
	delete y;*/

	return EXIT_SUCCESS;
}
