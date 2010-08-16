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

/*#include <esc/common.h>
#include <esc/debug.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <memory>*/
//#include <ustl.h>
//#include <iostream>
#include <iostream>
#include <stdlib.h>
#include <cmdargs.h>

//using namespace std;

#if 0
class my {
private:
	u32 abc;
public:
	my() : abc(4) {
		cout << "Constructor for " << this << endl;
	};
	~my() {
		cout << "Destructor for " << this << endl;
		abc = 0;
	};

	void doIt();
};

class A {
public:
	virtual ~A() { cout << "Destructing A" << endl; }
};
class B : public A {
public:
	virtual ~B() { cout << "Destructing B" << endl; }
};
class C : public B {
public:
	virtual ~C() { cout << "Destructing C" << endl; }
	inline void myMethod() { cout << "hi!" << endl; }
};

my myobj;

void my::doIt() {
	cout << "Ich bins: " << abc << endl;
}
#endif

class foo : public exception {
public:
	foo(const string& msg) : _msg(msg) {}
	virtual ~foo() throw() {}
	virtual const char* what() const throw() {
		return _msg.c_str();
	}

private:
	string _msg;
};

static void myfunc() {
	throw cmdargs_error("test");
}

int main(int argc,char **argv) {
	string fields;
	cmdargs args(argc,argv,0);
	try {
		//args.parse("f=s*",&fields);
		throw cmdargs_error("foo");
	}
	catch(const cmdargs_error& f) {
		cerr << "Catched exception" << endl;
	}
	catch(...) {
		cerr << "Catchall" << endl;
	}
	/*string fields;
	cmdargs args(argc,argv,0);
	try {
		args.parse("f=s*",&fields);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
	}*/

#if 0
	int x;
	int i = 0;
	while(cin >> x) {
		++i;
		cout << "Entered: " << x << endl;
	}
	cout << "Done (" << i << ")" << endl;
	cin.clear();

	auto_ptr<A> a(new C());
	C *c = dynamic_cast<C*>(a.get());
	if(c) {
		cout << "Worked" << endl;
		c->myMethod();
	}
	else {
		cout << "Didn't work!" << endl;
	}

	auto_ptr<A> a2(new B());
	C *c2 = dynamic_cast<C*>(a2.get());
	if(c2) {
		cout << "Worked!" << endl;
		c2->myMethod();
	}
	else {
		cout << "Didn't work!" << endl;
	}

	string name;
	cout << "Enter your Name: ";
	getline(cin,name);
	cout << "Hi " << name << "!" << endl;

	string abc;
	abc.append("test");
	string def(abc);
	cout << "len=" << def.length() << ", cap=" << def.capacity() << ", str=" << def.c_str() << endl;

	//vector<int> v;
    //v.resize(30);
    //for (size_t i = 0; i < v.capacity(); ++ i)
    //	v[i] = i;
    //v.push_back(57);
    //vector<int>::iterator vit = v.begin();
    //advance(vit,10);
    //v.insert (v.begin() + 20, 555);
    //v.erase (v.begin() + 3);

    //cout << "Hello world!" << endl;
    //cout << 456 << ios::hex << 0x1234 << endl;
    //cerr.format ("You objects are at 0x%08X\n", &v);*/

#if 0
    list<int> l;
    l.insert(l.begin(),10);
    l.insert(l.begin(),11);
    cout << "size=" << l.size() << endl;
    for(list<int>::iterator it = l.begin(); it != l.end(); it++)
    	cout << "	" << *it << endl;

    try {
    	throw 3;
    	cout << "hier" << endl;
    }
    catch(int &e) {
    	cout << "Got " << e << endl;
    }
    cout << "da" << endl;

	const string teststr("123 456 abc 1 myteststringfoo");
	char buf[10];
	istringstream istr(teststr);
	int i1,i2,i3;
	bool b1;
	istr >> i1 >> i2 >> hex >> i3 >> dec >> b1;
	istr.width(10);
	istr >> buf;
	cout.format("i1=%d, i2=%4d, i3=%#-8x, b1=%d buf='%s'\n",i1,i2,i3,b1,buf);

	ostringstream ostr;
	char bla[] = "foo";
	ostr << 1234 << ' ';
	ostr << 14u << ' ';
	ostr << &ostr << ' ';
	ostr << 1.412f << ' ';
	ostr << 1.412 << bla;
	ostr << 1.412L << endl;
	ostr.width(10);
	ostr << hex << showbase << '\'' << 0xabcdefu << '\'';
	cout << "str=" << ostr.str() << endl;

	ofstream f;
	f.open("/myfile");
	if(f.is_open())
		f << "foo" << "bar" << endl;
	else
		cerr << "Unable to open '/myfile'" << endl;
	f.close();

	ifstream f2;
	f2.open("/myfile");
	if(f2.is_open()) {
		char fbuf[10];
		f2.width(10);
		f2 >> fbuf;
		cout << "myfile='" << fbuf << "'" << endl;
	}
	else
		cerr << "Unable to open '/myfile'" << endl;
	f2.close();
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
#endif
	return 0;
}
