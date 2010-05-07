/**
 * D header file for C99.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */
module tango.stdc.stdlib;

private import tango.stdc.stddef;
private import tango.stdc.config;

extern (C):

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

const EXIT_SUCCESS  = 0;
const EXIT_FAILURE  = 1;
const RAND_MAX      = 32767;
const MB_CUR_MAX    = 1;

//TODO double  atof(in char* nptr);
int     atoi(in char* nptr);
c_long  atol(in char* nptr);
//TODO long    atoll(in char* nptr);

double  strtod(in char* nptr, char** endptr);
float   strtof(in char* nptr, char** endptr);
real    strtold(in char* nptr, char** endptr);
c_long  strtol(in char* nptr, char** endptr, int base);
long    strtoll(in char* nptr, char** endptr, int base);
c_ulong strtoul(in char* nptr, char** endptr, int base);
ulong   strtoull(in char* nptr, char** endptr, int base);

//TODO double  wcstod(in wchar_t* nptr, wchar_t** endptr);
//TODO float   wcstof(in wchar_t* nptr, wchar_t** endptr);
//TODO real    wcstold(in wchar_t* nptr, wchar_t** endptr);
//TODO c_long  wcstol(in wchar_t* nptr, wchar_t** endptr, int base);
//TODO long    wcstoll(in wchar_t* nptr, wchar_t** endptr, int base);
//TODO c_ulong wcstoul(in wchar_t* nptr, wchar_t** endptr, int base);
//TODO ulong   wcstoull(in wchar_t* nptr, wchar_t** endptr, int base);

int     rand();
void    srand(uint seed);

void*   malloc(size_t size);
void*   calloc(size_t nmemb, size_t size);
void*   realloc(void* ptr, size_t size);
void    free(void* ptr);

void    abort();
void    exit(int status);
int     atexit(void function() func);
//TODO void    _Exit(int status);

char*   getenv(in char* name);
int     system(in char* string);

void*   bsearch(in void* key, in void* base, size_t nmemb, size_t size, int function(in void*, in void*) compar);
void    qsort(void* base, size_t nmemb, size_t size, int function(in void*, in void*) compar);

int     abs(int j);
c_long  labs(c_long j);
//TODO long    llabs(long j);

div_t   div(int numer, int denom);
ldiv_t  ldiv(c_long numer, c_long denom);
//TODO lldiv_t lldiv(long numer, long denom);

//TODO int     mblen(in char* s, size_t n);
//TODO int     mbtowc(wchar_t* pwc, in char* s, size_t n);
//TODO int     wctomb(char*s, wchar_t wc);
//TODO size_t  mbstowcs(wchar_t* pwcs, in char* s, size_t n);
//TODO size_t  wcstombs(char* s, in wchar_t* pwcs, size_t n);

version( DigitalMars )
{
    void* alloca(size_t size);
}
else version( LDC )
{
    pragma(alloca)
        void* alloca(size_t size);
}
else version( GNU )
{
    private import gcc.builtins;
    alias gcc.builtins.__builtin_alloca alloca;
}
