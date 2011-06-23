/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ooc.h"

typedef struct sString sString;
typedef struct ClassString ClassString;

static sString *string_ctor(void *size);
static void string_dtor(sString *s);
static void string_append(sString *s,char c);
static void string_appends(sString *s,const char *str);

struct sString {
	const ClassString *class;
	char *str;
	size_t len;
	size_t size;
};
struct ClassString {
	size_t size;
	sString *(*ctor)(void *size);
	void (*dtor)(sString *s);
	void (*append)(sString *s,char c);
	void (*appends)(sString *s,const char *str);
} String = {
	.size = sizeof(sString),
	.ctor = &string_ctor,
	.dtor = &string_dtor,
	.append = &string_append,
	.appends = &string_appends
};

typedef struct sMyString sMyString;
typedef struct ClassMyString ClassMyString;

static sMyString *mystring_ctor(void *size);
static void mystring_dtor(sMyString *s);
static void mystring_append(sString *s,char c);
static void mystring_insert(sMyString *s,size_t index,char c);

struct sMyString {
	const ClassMyString *class;
	char *str;
	size_t len;
	size_t size;
	size_t stats;
};
struct ClassMyString {
	size_t size;
	const ClassString *super;
	sMyString *(*ctor)(void *size);
	void (*dtor)(sMyString *s);
	void (*append)(sString *s,char c);
	void (*appends)(sString *s,const char *str);
	void (*insert)(sMyString *s,size_t index,char c);
} MyString = {
	.size = sizeof(sMyString),
	.super = &String,
	.ctor = &mystring_ctor,
	.dtor = &mystring_dtor,
	.append = &mystring_append,
	.appends = &string_appends,
	.insert = &mystring_insert
};

#define new(class)	class.ctor(&class)
#define delete(obj)	obj->class->dtor(obj)
#define super(obj)	obj->class->super

static sString *string_ctor(void *size) {
	ClassString *class = (ClassString*)size;
	sString *s = (sString*)malloc(class->size);
	s->class = class;
	s->len = 0;
	s->size = 0;
	s->str = NULL;
	return s;
}

static void string_dtor(sString *s) {
	free(s->str);
	free(s);
}

static void string_append(sString *s,char c) {
	printf("%s: Appending %c to %p\n",__FUNCTION__,c,s);
}

static void string_appends(sString *s,const char *str) {
	ClassString *this = (ClassString*)s->class;
	while(*str)
		this->append(s,*str++);
}

static sMyString *mystring_ctor(void *size) {
	sMyString *s = (sMyString*)MyString.super->ctor(size);
	s->stats = 0;
	return s;
}

static void mystring_dtor(sMyString *s) {
	super(s)->dtor((sString*)s);
}

static void mystring_append(sString *s,char c) {
	printf("%s: Appending %c to %p\n",__FUNCTION__,c,s);
}

static void mystring_insert(sMyString *s,size_t index,char c) {
	UNUSED(index);
	ClassMyString *this = (ClassMyString*)s->class;
	this->append((sString*)s,c);
}

int mod_ooc(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	sString *str = new(String);
	String.appends(str,"foo");

	sMyString *my = new(MyString);
	MyString.insert(my,0,'c');
	delete(my);

	delete(str);
	return EXIT_SUCCESS;
}
