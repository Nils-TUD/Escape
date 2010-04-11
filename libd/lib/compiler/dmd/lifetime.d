/**
 * This module contains all functions related to an object's lifetime:
 * allocation, resizing, deallocation, and finalization.
 *
 * Copyright: Copyright (C) 2004-2007 Digital Mars, www.digitalmars.com.
 *            All rights reserved.
 * License:
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, in both source and binary form, subject to the following
 *  restrictions:
 *
 *  o  The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  o  Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  o  This notice may not be removed or altered from any source
 *     distribution.
 * Authors:   Walter Bright, Sean Kelly
 */
module lifetime;

//debug=LIFETIME;

private
{
    import tango.stdc.stdlib;
    import tango.stdc.string;
    import tango.stdc.stdarg;
    import tango.stdc.stdbool; // TODO: remove this when the old bit code goes away
    debug import tango.stdc.stdio;
}


private
{
    enum BlkAttr : uint
    {
        FINALIZE = 0b0000_0001,
        NO_SCAN  = 0b0000_0010,
        NO_MOVE  = 0b0000_0100,
        ALL_BITS = 0b1111_1111
    }

    extern (C) uint gc_getAttr( void* p );
    extern (C) uint gc_setAttr( void* p, uint a );
    extern (C) uint gc_clrAttr( void* p, uint a );

    extern (C) void* gc_malloc( size_t sz, uint ba = 0 );
    extern (C) void* gc_calloc( size_t sz, uint ba = 0 );
    extern (C) void  gc_free( void* p );

    extern (C) size_t gc_sizeOf( void* p );

    extern (C) bool onCollectResource( Object o );
    extern (C) void onFinalizeError( ClassInfo c, Exception e );
    extern (C) void onOutOfMemoryError();

    extern (C) void _d_monitorrelease(Object h);
}


/**
 *
 */
extern (C) Object _d_newclass(ClassInfo ci)
{
    void* p;

    debug(LIFETIME) printf("_d_newclass(ci = %p, %s)\n", ci, cast(char *)ci.name);
    if (ci.flags & 1) // if COM object
    {
        p = cast(void*)tango.stdc.stdlib.malloc(ci.init.length);
        if (!p)
            onOutOfMemoryError();
    }
    else
    {
        p = gc_malloc(ci.init.length,
                      BlkAttr.FINALIZE | (ci.flags & 2 ? BlkAttr.NO_SCAN : 0));
        debug(LIFETIME) printf(" p = %p\n", p);
    }

    debug(LIFETIME)
    {
        printf("p = %p\n", p);
        printf("ci = %p, ci.init = %p, len = %d\n", ci, ci.init, ci.init.length);
        printf("vptr = %p\n", *cast(void**) ci.init);
        printf("vtbl[0] = %p\n", (*cast(void***) ci.init)[0]);
        printf("vtbl[1] = %p\n", (*cast(void***) ci.init)[1]);
        printf("init[0] = %x\n", (cast(uint*) ci.init)[0]);
        printf("init[1] = %x\n", (cast(uint*) ci.init)[1]);
        printf("init[2] = %x\n", (cast(uint*) ci.init)[2]);
        printf("init[3] = %x\n", (cast(uint*) ci.init)[3]);
        printf("init[4] = %x\n", (cast(uint*) ci.init)[4]);
    }

    // initialize it
    (cast(byte*) p)[0 .. ci.init.length] = ci.init[];

    debug(LIFETIME) printf("initialization done\n");
    return cast(Object) p;
}


/**
 *
 */
extern (C) void _d_delinterface(void** p)
{
    if (*p)
    {
        Interface* pi = **cast(Interface ***)*p;
        Object     o  = cast(Object)(*p - pi.offset);

        _d_delclass(&o);
        *p = null;
    }
}


// used for deletion
private extern (D) alias void (*fp_t)(Object);


/**
 *
 */
extern (C) void _d_delclass(Object* p)
{
    if (*p)
    {
        debug(LIFETIME) printf("_d_delclass(%p)\n", *p);

        cr_finalize(cast(void*)*p);

        ClassInfo **pc = cast(ClassInfo **)*p;
        if (*pc)
        {
            ClassInfo c = **pc;

            if (c.deallocator)
            {
                fp_t fp = cast(fp_t)c.deallocator;
                (*fp)(*p); // call deallocator
                *p = null;
                return;
            }
        }
        gc_free(cast(void*)*p);
        *p = null;
    }
}


/**
 * Allocate a new array of length elements.
 * ti is the type of the resulting array, or pointer to element.
 * (For when the array is initialized to 0)
 */
extern (C) ulong _d_newarrayT(TypeInfo ti, size_t length)
{
    void* p;
    ulong result;
    auto size = ti.next.tsize();                // array element size

    debug(LIFETIME) printf("_d_newT(length = %d, size = %d)\n", length, size);
    if (length == 0 || size == 0)
        result = 0;
    else
    {
        size *= length;
        p = gc_malloc(size + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        debug(LIFETIME) printf(" p = %p\n", p);
        memset(p, 0, size);
        result = cast(ulong)length + (cast(ulong)cast(uint)p << 32);
    }
    return result;
}


/**
 * For when the array has a non-zero initializer.
 */
extern (C) ulong _d_newarrayiT(TypeInfo ti, size_t length)
{
    ulong result;
    auto size = ti.next.tsize();                // array element size

    debug(LIFETIME) printf("_d_newarrayii(length = %d, size = %d)\n", length, size);

    if (length == 0 || size == 0)
        result = 0;
    else
    {
        auto initializer = ti.next.init();
        auto isize = initializer.length;
        auto q = initializer.ptr;
        size *= length;
        auto p = gc_malloc(size + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        debug(LIFETIME) printf(" p = %p\n", p);
        if (isize == 1)
            memset(p, *cast(ubyte*)q, size);
        else if (isize == int.sizeof)
        {
            int init = *cast(int*)q;
            size /= int.sizeof;
            for (size_t u = 0; u < size; u++)
            {
                (cast(int*)p)[u] = init;
            }
        }
        else
        {
            for (size_t u = 0; u < size; u += isize)
            {
                memcpy(p + u, q, isize);
            }
        }
        va_end(q);
        result = cast(ulong)length + (cast(ulong)cast(uint)p << 32);
    }
    return result;
}


/**
 *
 */
extern (C) ulong _d_newarraymT(TypeInfo ti, int ndims, ...)
{
    ulong result;

    debug(LIFETIME) printf("_d_newarraymT(ndims = %d)\n", ndims);
    if (ndims == 0)
        result = 0;
    else
    {   va_list q;
        va_start!(int)(q, ndims);

        void[] foo(TypeInfo ti, size_t* pdim, int ndims)
        {
            size_t dim = *pdim;
            void[] p;

            debug(LIFETIME)
            	printf("foo(ti = %p, ti.next = %p, dim = %d, ndims = %d\n", ti, ti.next, dim, ndims);
            if (ndims == 1)
            {
                auto r = _d_newarrayT(ti, dim);
                p = *cast(void[]*)(&r);
            }
            else
            {
                p = gc_malloc(dim * (void[]).sizeof + 1)[0 .. dim];
                for (int i = 0; i < dim; i++)
                {
                    (cast(void[]*)p.ptr)[i] = foo(ti.next, pdim + 1, ndims - 1);
                }
            }
            return p;
        }

        size_t* pdim = cast(size_t *)q;
        result = cast(ulong)foo(ti, pdim, ndims);
        debug(LIFETIME) printf("result = %llx\n", result);

        version (none)
        {
            for (int i = 0; i < ndims; i++)
            {
                printf("index %d: %d\n", i, va_arg!(int)(q));
            }
        }
        va_end(q);
    }
    return result;
}


/**
 *
 */
extern (C) ulong _d_newarraymiT(TypeInfo ti, int ndims, ...)
{
    ulong result;

    debug(LIFETIME) printf("_d_newarraymi(ndims = %d)\n", ndims);
    if (ndims == 0)
        result = 0;
    else
    {
        va_list q;
        va_start!(int)(q, ndims);

        void[] foo(TypeInfo ti, size_t* pdim, int ndims)
        {
            size_t dim = *pdim;
            void[] p;

            if (ndims == 1)
                {
                auto r = _d_newarrayiT(ti, dim);
                p = *cast(void[]*)(&r);
            }
            else
            {
                p = gc_malloc(dim * (void[]).sizeof + 1)[0 .. dim];
                for (int i = 0; i < dim; i++)
                {
                    (cast(void[]*)p.ptr)[i] = foo(ti.next, pdim + 1, ndims - 1);
                }
            }
            return p;
        }

        size_t* pdim = cast(size_t *)q;
        result = cast(ulong)foo(ti, pdim, ndims);
        debug(LIFETIME) printf("result = %llx\n", result);

        version (none)
        {
            for (int i = 0; i < ndims; i++)
            {
                printf("index %d: %d\n", i, va_arg!(int)(q));
                printf("init = %d\n", va_arg!(int)(q));
            }
        }
        va_end(q);
    }
    return result;
}


/**
 *
 */
struct Array
{
    size_t length;
    byte*  data;
}


/**
 *
 */
extern (C) void _d_delarray(Array *p)
{
    if (p)
    {
        assert(!p.length || p.data);

        if (p.data)
            gc_free(p.data);
        p.data = null;
        p.length = 0;
    }
}


/**
 *
 */
extern (C) void _d_delmemory(void* *p)
{
    if (*p)
    {
        gc_free(*p);
        *p = null;
    }
}


/**
 *
 */
extern (C) void _d_callfinalizer(void* p)
{
    cr_finalize( p );
}


/**
 *
 */
extern (C) void cr_finalize(void* p, bool det = true)
{
    debug(LIFETIME) printf("cr_finalize(p = %p)\n", p);

    if (p) // not necessary if called from gc
    {
        ClassInfo** pc = cast(ClassInfo**)p;

        if (*pc)
        {
            ClassInfo c = **pc;

            try
            {
                if (det || onCollectResource(cast(Object)p))
                {
                    do
                    {
                        if (c.destructor)
                        {
                            fp_t fp = cast(fp_t)c.destructor;
                            (*fp)(cast(Object)p); // call destructor
                        }
                        c = c.base;
                    } while (c);
                }
                if ((cast(void**)p)[1]) // if monitor is not null
                    _d_monitorrelease(cast(Object)p);
            }
            catch (Exception e)
            {
                onFinalizeError(**pc, e);
            }
            finally
            {
                *pc = null; // zero vptr
            }
        }
    }
}


/**
 * Resize dynamic arrays with 0 initializers.
 */
extern (C) byte[] _d_arraysetlengthT(TypeInfo ti, size_t newlength, Array *p)
in
{
    assert(ti);
    assert(!p.length || p.data);
}
body
{
    byte* newdata;
    size_t sizeelem = ti.next.tsize();

    debug(LIFETIME)
    {
        printf("_d_arraysetlengthT(p = %p, sizeelem = %d, newlength = %d)\n", p, sizeelem, newlength);
        if (p)
            printf("\tp.data = %p, p.length = %d\n", p.data, p.length);
    }

    if (newlength)
    {
        version (D_InlineAsm_X86)
        {
            size_t newsize = void;

            asm
            {
                mov EAX, newlength;
                mul EAX, sizeelem;
                mov newsize, EAX;
                jc  Loverflow;
            }
        }
        else
        {
            size_t newsize = sizeelem * newlength;

            if (newsize / newlength != sizeelem)
                goto Loverflow;
        }

        debug(LIFETIME) printf("newsize = %x, newlength = %x\n", newsize, newlength);

        if (p.data)
        {
            newdata = p.data;
            if (newlength > p.length)
            {
                size_t size = p.length * sizeelem;
                size_t cap  = gc_sizeOf(p.data);

                if (cap <= newsize)
                {
                    newdata = cast(byte *)gc_malloc(newsize + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
                    newdata[0 .. size] = p.data[0 .. size];
                }
                newdata[size .. newsize] = 0;
            }
        }
        else
        {
            newdata = cast(byte *)gc_calloc(newsize + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        }
    }
    else
    {
        newdata = p.data;
    }

    p.data = newdata;
    p.length = newlength;
    return newdata[0 .. newlength];

Loverflow:
    onOutOfMemoryError();
}


/**
 * Resize arrays for non-zero initializers.
 *      p               pointer to array lvalue to be updated
 *      newlength       new .length property of array
 *      sizeelem        size of each element of array
 *      initsize        size of initializer
 *      ...             initializer
 */
extern (C) byte[] _d_arraysetlengthiT(TypeInfo ti, size_t newlength, Array *p)
in
{
    assert(!p.length || p.data);
}
body
{
    byte* newdata;
    size_t sizeelem = ti.next.tsize();
    void[] initializer = ti.next.init();
    size_t initsize = initializer.length;

    assert(sizeelem);
    assert(initsize);
    assert(initsize <= sizeelem);
    assert((sizeelem / initsize) * initsize == sizeelem);

    debug(LIFETIME)
    {
        printf("_d_arraysetlengthiT(p = %p, sizeelem = %d, newlength = %d, initsize = %d)\n", p, sizeelem, newlength, initsize);
        if (p)
            printf("\tp.data = %p, p.length = %d\n", p.data, p.length);
    }

    if (newlength)
    {
        version (D_InlineAsm_X86)
        {
            size_t newsize = void;

            asm
            {
                mov EAX, newlength;
                mul EAX, sizeelem;
                mov newsize, EAX;
                jc  Loverflow;
            }
        }
        else
        {
            size_t newsize = sizeelem * newlength;

            if (newsize / newlength != sizeelem)
                goto Loverflow;
        }
        debug(LIFETIME) printf("newsize = %x, newlength = %x\n", newsize, newlength);

        size_t size = p.length * sizeelem;

        if (p.data)
        {
            newdata = p.data;
            if (newlength > p.length)
            {
                size_t cap = gc_sizeOf(p.data);

                if (cap <= newsize)
                {
                    newdata = cast(byte *)gc_malloc(newsize + 1);
                    newdata[0 .. size] = p.data[0 .. size];
                }
            }
        }
        else
        {
            newdata = cast(byte *)gc_malloc(newsize + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        }

        auto q = initializer.ptr;       // pointer to initializer

        if (newsize > size)
        {
            if (initsize == 1)
            {
                debug(LIFETIME) printf("newdata = %p, size = %d, newsize = %d, *q = %d\n", newdata, size, newsize, *cast(byte*)q);
                newdata[size .. newsize] = *(cast(byte*)q);
            }
            else
            {
                for (size_t u = size; u < newsize; u += initsize)
                {
                    memcpy(newdata + u, q, initsize);
                }
            }
        }
    }
    else
    {
        newdata = p.data;
    }

    p.data = newdata;
    p.length = newlength;
    return newdata[0 .. newlength];

Loverflow:
    onOutOfMemoryError();
}


/**
 * Append y[] to array x[].
 * size is size of each array element.
 */
extern (C) long _d_arrayappendT(TypeInfo ti, Array *px, byte[] y)
{
    auto size = ti.next.tsize();                // array element size
    size_t cap = gc_sizeOf(px.data);
    size_t length    = px.length;
    size_t newlength = length + y.length;

    if (newlength * size > cap)
    {   byte* newdata;

        newdata = cast(byte *)gc_malloc(newCapacity(newlength, size) + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        memcpy(newdata, px.data, length * size);
        px.data = newdata;
    }
    px.length = newlength;
    memcpy(px.data + length * size, y.ptr, y.length * size);
    return *cast(long*)px;
}


/**
 *
 */
size_t newCapacity(size_t newlength, size_t size)
{
    version(none)
    {
        size_t newcap = newlength * size;
    }
    else
    {
        /*
         * Better version by Dave Fladebo:
         * This uses an inverse logorithmic algorithm to pre-allocate a bit more
         * space for larger arrays.
         * - Arrays smaller than 4096 bytes are left as-is, so for the most
         * common cases, memory allocation is 1 to 1. The small overhead added
         * doesn't effect small array perf. (it's virutally the same as
         * current).
         * - Larger arrays have some space pre-allocated.
         * - As the arrays grow, the relative pre-allocated space shrinks.
         * - The logorithmic algorithm allocates relatively more space for
         * mid-size arrays, making it very fast for medium arrays (for
         * mid-to-large arrays, this turns out to be quite a bit faster than the
         * equivalent realloc() code in C, on Linux at least. Small arrays are
         * just as fast as GCC).
         * - Perhaps most importantly, overall memory usage and stress on the GC
         * is decreased significantly for demanding environments.
         */
        size_t newcap = newlength * size;
        size_t newext = 0;

        if (newcap > 4096)
        {
            //double mult2 = 1.0 + (size / log10(pow(newcap * 2.0,2.0)));

            // redo above line using only integer math

            static int log2plus1(size_t c)
            {   int i;

                if (c == 0)
                    i = -1;
                else
                    for (i = 1; c >>= 1; i++)
                    {
                    }
                return i;
            }

            /* The following setting for mult sets how much bigger
             * the new size will be over what is actually needed.
             * 100 means the same size, more means proportionally more.
             * More means faster but more memory consumption.
             */
            //long mult = 100 + (1000L * size) / (6 * log2plus1(newcap));
            long mult = 100 + (1000L * size) / log2plus1(newcap);

            // testing shows 1.02 for large arrays is about the point of diminishing return
            if (mult < 102)
                mult = 102;
            newext = cast(size_t)((newcap * mult) / 100);
            newext -= newext % size;
            debug(LIFETIME) printf("mult: %2.2f, alloc: %2.2f\n",mult/100.0,newext / cast(double)size);
        }
        newcap = newext > newcap ? newext : newcap;
        debug(LIFETIME) printf("newcap = %d, newlength = %d, size = %d\n", newcap, newlength, size);
    }
    return newcap;
}


/**
 *
 */
extern (C) byte[] _d_arrayappendcT(TypeInfo ti, inout byte[] x, ...)
{
    auto size = ti.next.tsize();                // array element size
    size_t cap = gc_sizeOf(x.ptr);
    size_t length    = x.length;
    size_t newlength = length + 1;

    assert(cap == 0 || length * size <= cap);

    debug(LIFETIME) printf("_d_arrayappendc(size = %d, ptr = %p, length = %d, cap = %d)\n", size, x.ptr, x.length, cap);

    if (newlength * size >= cap)
    {   byte* newdata;

        debug(LIFETIME) printf("_d_arrayappendc(size = %d, newlength = %d, cap = %d)\n", size, newlength, cap);
        cap = newCapacity(newlength, size);
        assert(cap >= newlength * size);
        newdata = cast(byte *)gc_malloc(cap + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        memcpy(newdata, x.ptr, length * size);
        (cast(void**)(&x))[1] = newdata;
    }
    byte *argp = cast(byte *)(&ti + 2);

    *cast(size_t *)&x = newlength;
    (cast(byte *)x)[length * size .. newlength * size] = argp[0 .. size];
    assert((cast(size_t)x.ptr & 15) == 0);
    assert(gc_sizeOf(x.ptr) > x.length * size);
    return x;
}


/**
 *
 */
extern (C) byte[] _d_arraycatT(TypeInfo ti, byte[] x, byte[] y)
out (result)
{
    auto size = ti.next.tsize();                // array element size
    debug(LIFETIME) printf("_d_arraycatT(%d,%p ~ %d,%p size = %d => %d,%p)\n", x.length, x.ptr, y.length, y.ptr, size, result.length, result.ptr);
    assert(result.length == x.length + y.length);
    for (size_t i = 0; i < x.length * size; i++)
        assert((cast(byte*)result)[i] == (cast(byte*)x)[i]);
    for (size_t i = 0; i < y.length * size; i++)
        assert((cast(byte*)result)[x.length * size + i] == (cast(byte*)y)[i]);

    size_t cap = gc_sizeOf(result.ptr);
    assert(!cap || cap > result.length * size);
}
body
{
    version (none)
    {
        /* Cannot use this optimization because:
         *  char[] a, b;
         *  char c = 'a';
         *  b = a ~ c;
         *  c = 'b';
         * will change the contents of b.
         */
        if (!y.length)
            return x;
        if (!x.length)
            return y;
    }

    debug(LIFETIME) printf("_d_arraycatT(%d,%p ~ %d,%p)\n", x.length, x.ptr, y.length, y.ptr);
    auto size = ti.next.tsize();                // array element size
    debug(LIFETIME) printf("_d_arraycatT(%d,%p ~ %d,%p size = %d)\n", x.length, x.ptr, y.length, y.ptr, size);
    size_t xlen = x.length * size;
    size_t ylen = y.length * size;
    size_t len  = xlen + ylen;

    if (!len)
        return null;

    byte* p = cast(byte*)gc_malloc(len + 1, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
    memcpy(p, x.ptr, xlen);
    memcpy(p + xlen, y.ptr, ylen);
    p[len] = 0;
    return p[0 .. x.length + y.length];
}


/**
 *
 */
extern (C) byte[] _d_arraycatnT(TypeInfo ti, uint n, ...)
{   byte[] a;
    size_t length;
    byte[]* p;
    uint i;
    byte[] b;
    auto size = ti.next.tsize();                // array element size

    p = cast(byte[]*)(&n + 1);

    for (i = 0; i < n; i++)
{
        b = *p++;
        length += b.length;
    }
    if (!length)
        return null;

    a = new byte[length * size];
    if (!(ti.next.flags() & 1))
        gc_setAttr(a.ptr, BlkAttr.NO_SCAN);
    p = cast(byte[]*)(&n + 1);

    uint j = 0;
    for (i = 0; i < n; i++)
    {
        b = *p++;
        if (b.length)
        {
            memcpy(&a[j], b.ptr, b.length * size);
            j += b.length * size;
        }
    }

    *cast(int *)&a = length;    // jam length
    //a.length = length;
    return a;
}


/**
 *
 */
extern (C) void* _d_arrayliteralT(TypeInfo ti, size_t length, ...)
{
    auto size = ti.next.tsize();                // array element size
    byte[] result;

    debug(LIFETIME) printf("_d_arrayliteralT(size = %d, length = %d)\n", size, length);
    if (length == 0 || size == 0)
        result = null;
    else
    {
        result = new byte[length * size];
        if (!(ti.next.flags() & 1))
            gc_setAttr(result.ptr, BlkAttr.NO_SCAN);
        *cast(size_t *)&result = length;        // jam length

        va_list q;
        va_start!(size_t)(q, length);

        size_t stacksize = (size + int.sizeof - 1) & ~(int.sizeof - 1);

        if (stacksize == size)
        {
            memcpy(result.ptr, q, length * size);
        }
        else
        {
            for (size_t i = 0; i < length; i++)
            {
                memcpy(result.ptr + i * size, q, size);
                q += stacksize;
        }
    }

        va_end(q);
    }
    return result.ptr;
}


/**
 * Support for array.dup property.
 */
struct Array2
{
    size_t length;
    void* ptr;
}


/**
 *
 */
extern (C) long _adDupT(TypeInfo ti, Array2 a)
out (result)
{
    auto szelem = ti.next.tsize();          // array element size
    assert(memcmp((*cast(Array2*)&result).ptr, a.ptr, a.length * szelem) == 0);
}
body
{
    Array2 r;

    if (a.length)
    {
        auto szelem = ti.next.tsize();              // array element size
        auto size = a.length * szelem;
        r.ptr = gc_malloc(size, !(ti.next.flags() & 1) ? BlkAttr.NO_SCAN : 0);
        r.length = a.length;
        memcpy(r.ptr, a.ptr, size);
    }
    return *cast(long*)(&r);
}

unittest
{
    int[] a;
    int[] b;
    int i;

    a = new int[3];
    a[0] = 1; a[1] = 2; a[2] = 3;
    b = a.dup;
    assert(b.length == 3);
    for (i = 0; i < 3; i++)
        assert(b[i] == i + 1);
}
