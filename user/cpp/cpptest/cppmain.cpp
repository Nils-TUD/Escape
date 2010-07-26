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

#include <esc/common.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <istream>
#include <sstream>
//#include <ustl.h>

using namespace std;

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

class A {
public:
	virtual ~A() {}
};
class B : public A {
public:
	virtual ~B() {}
};
class C : public B {
public:
	virtual ~C() {}
	inline void myMethod() { printf("hi!\n"); }
};

my myobj;

void my::doIt() {
	printf("Ich bins: %d\n",abc);
}

int main(void) {
	A *a = new C();
	C *c = dynamic_cast<C*>(a);
	if(c) {
		printf("Worked!\n");
		c->myMethod();
	}
	else {
		printf("Didn't work!\n");
	}

	A *a2 = new B();
	C *c2 = dynamic_cast<C*>(a2);
	if(c2) {
		printf("Worked!\n");
		c2->myMethod();
	}
	else {
		printf("Didn't work!\n");
	}

	delete[] (void*)0;

	string abc;
	abc.append("test");
	string def(abc);
	printf("len=%d, cap=%d, str=%s\n",def.length(),def.capacity(),def.c_str());

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
    v.resize(30);
    for (size_t i = 0; i < v.capacity(); ++ i)
    	v[i] = i;
    v.push_back(57);
    vector<int>::iterator vit = v.begin();
    advance(vit,10);
    //v.insert (v.begin() + 20, 555);
    //v.erase (v.begin() + 3);

    //cout << "Hello world!" << endl;
    //cout << 456 << ios::hex << 0x1234 << endl;
    //cerr.format ("You objects are at 0x%08X\n", &v);*/

    list<int> l;
    l.insert(l.begin(),10);
    l.insert(l.begin(),11);
    printf("size=%d\n",l.size());
    for(list<int>::iterator it = l.begin(); it != l.end(); it++)
    	printf(" %d\n",*it);

    try {
    	throw 3;
    	printf("hier\n");
    }
    catch(int &e) {
    	printf("Got %d\n",e);
    }
	printf("da\n");

	string teststr("123 456 abc 1");
	stringbuf buf(teststr);
	istream istr(&buf);
	int i1,i2,i3;
	bool b1;
	istr >> i1 >> i2 >> hex >> i3 >> dec >> b1;
	printf("i1=%d, i2=%d, i3=%d, b1=%d\n",i1,i2,i3,b1);

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
