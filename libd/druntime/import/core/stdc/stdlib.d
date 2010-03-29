/**
 * D header file for C99.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 *
 *          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
module core.stdc.stdlib;

private import core.stdc.config;
public import core.stdc.stddef; // for size_t, wchar_t

extern (C):
nothrow:

struct div_t
{
    int quot,
        rem;
}

struct ldiv_t
{
    int quot,
        rem;
}

struct lldiv_t
{
    long quot,
         rem;
}

enum EXIT_SUCCESS = 0;
enum EXIT_FAILURE = 1;
enum RAND_MAX     = 32767;
enum MB_CUR_MAX   = 1;

// TODO double  atof(in char* nptr);
int     atoi(in char* nptr);
c_long  atol(in char* nptr);
// TODO long    atoll(in char* nptr);

double  strtod(in char* nptr, char** endptr);
float   strtof(in char* nptr, char** endptr);
real    strtold(in char* nptr, char** endptr);
// TODO c_long  strtol(in char* nptr, char** endptr, int base);
// TODO long    strtoll(in char* nptr, char** endptr, int base);
// TODO c_ulong strtoul(in char* nptr, char** endptr, int base);
// TODO ulong   strtoull(in char* nptr, char** endptr, int base);

int     rand();
void    srand(uint seed);

void*   malloc(size_t size);
void*   calloc(size_t nmemb, size_t size);
void*   realloc(void* ptr, size_t size);
void    free(void* ptr);

void    abort();
void    exit(int status);
int     atexit(void function() func);
// TODO void    _Exit(int status);

char*   getenv(in char* name);
int     system(in char* string);

void*   bsearch(in void* key, in void* base, size_t nmemb, size_t size, int function(in void*, in void*) compar);
void    qsort(void* base, size_t nmemb, size_t size, int function(in void*, in void*) compar);

pure int     abs(int j);
pure c_long  labs(c_long j);
// TODO pure long    llabs(long j);

div_t   div(int numer, int denom);
ldiv_t  ldiv(c_long numer, c_long denom);
// TODO lldiv_t lldiv(long numer, long denom);

// TODO int     mblen(in char* s, size_t n);
// TODO int     mbtowc(wchar_t* pwc, in char* s, size_t n);
// TODO int     wctomb(char*s, wchar_t wc);
// TODO size_t  mbstowcs(wchar_t* pwcs, in char* s, size_t n);
// TODO size_t  wcstombs(char* s, in wchar_t* pwcs, size_t n);

version( DigitalMars )
{
    void* alloca(size_t size); // non-standard
}
