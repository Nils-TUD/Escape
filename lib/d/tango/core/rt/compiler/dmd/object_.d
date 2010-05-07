/**
 * Part of the D programming language runtime library.
 * Forms the symbols available to all D programs. Includes
 * Object, which is the root of the class object hierarchy.
 *
 * This module is implicitly imported.
 * Macros:
 *      WIKI = Object
 */

/*
 *  Copyright (C) 2004-2007 by Digital Mars, www.digitalmars.com
 *  Written by Walter Bright
 *
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
 */

/*
 *  Modified by Sean Kelly <sean@f4.ca> for use with Tango.
 */

module object;

private
{
    import tango.stdc.string : memcmp, memcpy, memmove;
    import tango.stdc.stdlib : calloc, realloc, free;
    import tango.stdc.stdio : snprintf;
    import tango.core.Exception : onOutOfMemoryError;
    import rt.compiler.util.string;
    import rt.compiler.util.hash;
    import rt.compiler.dmd.rt.aaA;
    debug(PRINTF) import tango.stdc.stdio: printf;
    extern (C) Object _d_newclass(ClassInfo ci);
}

// NOTE: For some reason, this declaration method doesn't work
//       in this particular file (and this file only).  It must
//       be a DMD thing.
//alias typeof(int.sizeof)                    size_t;
//alias typeof(cast(void*)0 - cast(void*)0)   ptrdiff_t;

version( X86_64 )
{
    alias ulong size_t;
    alias long  ptrdiff_t;
}
else
{
    alias uint  size_t;
    alias int   ptrdiff_t;
}

alias size_t hash_t;
alias int equals_t;

/**
 * All D class objects inherit from Object.
 */
class Object
{
    /**
     * Override this to capture an explicit delete or an implicit 
     * delete via a scoped-instance. Unlike a dtor(), GC references
     * are still intact when this method is invoked
     */
    void dispose()
    {
    }

    /**
     * Convert Object to a human readable string.
     */
    char[] toString()
    {
        return this.classinfo.name;
    }

    /**
     * Compute hash function for Object.
     */
    hash_t toHash()
    {
        // BUG: this prevents a compacting GC from working, needs to be fixed
        return cast(hash_t)cast(void*)this;
    }

    /**
     * Compare with another Object obj.
     * Returns:
     *  $(TABLE
     *  $(TR $(TD this &lt; obj) $(TD &lt; 0))
     *  $(TR $(TD this == obj) $(TD 0))
     *  $(TR $(TD this &gt; obj) $(TD &gt; 0))
     *  )
     */
    int opCmp(Object o)
    {
        // BUG: this prevents a compacting GC from working, needs to be fixed
        //return cast(int)cast(void*)this - cast(int)cast(void*)o;

        throw new Exception("need opCmp for class " ~ this.classinfo.name);
        //return ((cast(void*)this<cast(void*)o)?-1:((cast(void*)this==cast(void*)o)?0:1));
    }

    /**
     * Returns !=0 if this object does have the same contents as obj.
     */
    equals_t opEquals(Object o)
    {
        return cast(equals_t)(this is o);
    }

    interface Monitor
    {
        void lock();
        void unlock();
    }
}

/**
 * Information about an interface.
 * When an object is accessed via an interface, an Interface* appears as the
 * first entry in its vtbl.
 */
struct Interface
{
    ClassInfo   classinfo;  /// .classinfo for this interface (not for containing class)
    void*[]     vtbl;
    ptrdiff_t   offset;     /// offset to Interface 'this' from Object 'this'
}

/**
 * Runtime type information about a class. Can be retrieved for any class type
 * or instance by using the .classinfo property.
 * A pointer to this appears as the first entry in the class's vtbl[].
 */
class ClassInfo : Object
{
    byte[]      init;           /** class static initializer
                                 * (init.length gives size in bytes of class)
                                 */
    char[]      name;           /// class name
    void*[]     vtbl;           /// virtual function pointer table
    Interface[] interfaces;     /// interfaces this class implements
    ClassInfo   base;           /// base class
    void*       destructor;     // Only use as delegate.funcptr!
    void*       classInvariant; // Only use as delegate.funcptr!
    uint        flags;
    //  1:                      // is IUnknown or is derived from IUnknown
    //  2:                      // has no possible pointers into GC memory
    //  4:                      // has offTi[] member
    //  8:                      // has constructors
    //  32:         // has typeinfo
    void*       deallocator;
    OffsetTypeInfo[] offTi;
    Object function() defaultConstructor;   // default Constructor. Only use as delegate.funcptr!
    static if (__VERSION__ >= 1045) {
        TypeInfo typeinfo;
    }
    
    /**
     * Search all modules for ClassInfo corresponding to classname.
     * Returns: null if not found
     */
    static ClassInfo find(char[] classname)
    {
        foreach (m; ModuleInfo)
        {
            //writefln("module %s, %d", m.name, m.localClasses.length);
            foreach (c; m.localClasses)
            {
                //writefln("\tclass %s", c.name);
                if (c.name == classname)
                    return c;
            }
        }
        return null;
    }

    /**
     * Create instance of Object represented by 'this'.
     */
    Object create()
    {
        if (flags & 8 && defaultConstructor is null)
            return null;
        Object o = _d_newclass(this);
        if (flags & 8 && defaultConstructor !is null)
        {
            Object delegate() ctor;
            ctor.ptr = cast(void*)o;
            ctor.funcptr = cast(Object function())defaultConstructor;
            return ctor();
        }
        return o;
    }
}

/**
 * Array of pairs giving the offset and type information for each
 * member in an aggregate.
 */
struct OffsetTypeInfo
{
    size_t   offset;    /// Offset of member from start of object
    TypeInfo ti;        /// TypeInfo for this member
}

/**
 * Runtime type information about a type.
 * Can be retrieved for any type using a
 * <a href="../expression.html#typeidexpression">TypeidExpression</a>.
 */
class TypeInfo
{
    hash_t toHash()
    {   hash_t hash;

        foreach (char c; this.toString())
            hash = hash * 9 + c;
        return hash;
    }

    int opCmp(Object o)
    {
        if (this is o)
            return 0;
        TypeInfo ti = cast(TypeInfo)o;
        if (ti is null)
            return 1;
        return stringCompare(this.toString(), ti.toString());
    }

    override equals_t opEquals(Object o)
    {
        /* TypeInfo instances are singletons, but duplicates can exist
         * across DLL's. Therefore, comparing for a name match is
         * sufficient.
         */
        if (this is o)
            return 1;
        TypeInfo ti = cast(TypeInfo)o;
        return cast(equals_t)(ti && this.toString() == ti.toString());
    }

    /// Returns a hash of the instance of a type.
    hash_t getHash(in void* p) { return cast(hash_t)p; }

    /// Compares two instances for equality.
    equals_t equals(in void* p1, in void* p2) { return p1 == p2; }

    /// Compares two instances for &lt;, ==, or &gt;.
    int compare(in void* p1, in void* p2) { return 0; } // throw new Exception("non comparable",__FILE__,__LINE__);

    /// Returns size of the type.
    size_t tsize() { return 0; }

    /// Swaps two instances of the type.
    void swap(void* p1, void* p2)
    {
        size_t n = tsize();
        for (size_t i = 0; i < n; i++)
        {
            byte t = (cast(byte *)p1)[i];
            (cast(byte*)p1)[i] = (cast(byte*)p2)[i];
            (cast(byte*)p2)[i] = t;
        }
    }

    /// Get TypeInfo for 'next' type, as defined by what kind of type this is,
    /// null if none.
    TypeInfo next() { return null; }

    /// Return default initializer, null if default initialize to 0
    void[] init() { return null; }

    /// Get flags for type: 1 means GC should scan for pointers
    uint flags() { return 0; }

    /// Get type information on the contents of the type; null if not available
    OffsetTypeInfo[] offTi() { return null; }
}

class TypeInfo_Typedef : TypeInfo
{
    override char[] toString() { return name; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Typedef c;
        return this is o ||
               ((c = cast(TypeInfo_Typedef)o) !is null &&
                this.name == c.name &&
                this.base == c.base);
    }

    override hash_t getHash(in void* p) { return base.getHash(p); }
    override equals_t equals(in void* p1, in void* p2) { return base.equals(p1, p2); }
    override int compare(in void* p1, in void* p2) { return base.compare(p1, p2); }
    override size_t tsize() { return base.tsize(); }
    override void swap(void* p1, void* p2) { return base.swap(p1, p2); }

    override TypeInfo next() { return base; }
    override uint flags() { return base.flags(); }
    override void[] init() { return m_init.length ? m_init : base.init(); }

    TypeInfo base;
    char[] name;
    void[] m_init;
}

class TypeInfo_Enum : TypeInfo_Typedef
{

}

class TypeInfo_Pointer : TypeInfo
{
    override char[] toString() { return m_next.toString() ~ "*"; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Pointer c;
        return this is o ||
                ((c = cast(TypeInfo_Pointer)o) !is null &&
                 this.m_next == c.m_next);
    }

    override hash_t getHash(in void* p)
    {
        return cast(hash_t)*cast(void**)p;
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        return cast(equals_t)(*cast(void**)p1 == *cast(void**)p2);
    }

    override int compare(in void* p1, in void* p2)
    {
        if (*cast(void**)p1 < *cast(void**)p2)
            return -1;
        else if (*cast(void**)p1 > *cast(void**)p2)
            return 1;
        else
            return 0;
    }

    override size_t tsize()
    {
        return (void*).sizeof;
    }

    override void swap(void* p1, void* p2)
    {
        void* tmp = *cast(void**)p1;
        *cast(void**)p1 = *cast(void**)p2;
        *cast(void**)p2 = tmp;
    }

    override TypeInfo next() { return m_next; }
    override uint flags() { return 1; }

    TypeInfo m_next;
}

class TypeInfo_Array : TypeInfo
{
    override char[] toString() { return value.toString() ~ "[]"; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Array c;
        return this is o ||
               ((c = cast(TypeInfo_Array)o) !is null &&
                this.value == c.value);
    }

    override hash_t getHash(in void* p)
    {
        size_t sz = value.tsize();
        void[] a = *cast(void[]*)p;
        hash_t hash = a.length;
        for (size_t i = 0; i < a.length; i++)
            hash = rt_hash_combine(value.getHash(a.ptr + i * sz),hash);
        return hash;
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        void[] a1 = *cast(void[]*)p1;
        void[] a2 = *cast(void[]*)p2;
        if (a1.length != a2.length)
            return false;
        size_t sz = value.tsize();
        for (size_t i = 0; i < a1.length; i++)
        {
            if (!value.equals(a1.ptr + i * sz, a2.ptr + i * sz))
                return false;
        }
        return true;
    }

    override int compare(in void* p1, in void* p2)
    {
        void[] a1 = *cast(void[]*)p1;
        void[] a2 = *cast(void[]*)p2;
        size_t sz = value.tsize();
        size_t len = a1.length;

        if (a2.length < len)
            len = a2.length;
        for (size_t u = 0; u < len; u++)
        {
            int result = value.compare(a1.ptr + u * sz, a2.ptr + u * sz);
            if (result)
                return result;
        }
        return cast(int)a1.length - cast(int)a2.length;
    }

    override size_t tsize()
    {
        return (void[]).sizeof;
    }

    override void swap(void* p1, void* p2)
    {
        void[] tmp = *cast(void[]*)p1;
        *cast(void[]*)p1 = *cast(void[]*)p2;
        *cast(void[]*)p2 = tmp;
    }

    TypeInfo value;

    override TypeInfo next()
    {
        return value;
    }

    override uint flags() { return 1; }
}

class TypeInfo_StaticArray : TypeInfo
{
    override char[] toString()
    {
        char [10] tmp = void;
        return value.toString() ~ "[" ~ intToUtf8(tmp, len) ~ "]";
    }

    override equals_t opEquals(Object o)
    {
        TypeInfo_StaticArray c;
        return this is o ||
               ((c = cast(TypeInfo_StaticArray)o) !is null &&
                this.len == c.len &&
                this.value == c.value);
    }

    override hash_t getHash(in void* p)
    {
        size_t sz = value.tsize();
        hash_t hash = len;
        for (size_t i = 0; i < len; i++)
            hash = rt_hash_combine(value.getHash(p + i * sz),hash);
        return hash;
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        size_t sz = value.tsize();

        for (size_t u = 0; u < len; u++)
        {
            if (!value.equals(p1 + u * sz, p2 + u * sz))
                return false;
        }
        return true;
    }

    override int compare(in void* p1, in void* p2)
    {
        size_t sz = value.tsize();

        for (size_t u = 0; u < len; u++)
        {
            int result = value.compare(p1 + u * sz, p2 + u * sz);
            if (result)
                return result;
        }
        return 0;
    }

    override size_t tsize()
    {
        return len * value.tsize();
    }

    override void swap(void* p1, void* p2)
    {
        void* tmp;
        size_t sz = value.tsize();
        ubyte[16] buffer;
        void* pbuffer;

        if (sz < buffer.sizeof)
            tmp = buffer.ptr;
        else
            tmp = pbuffer = (new void[sz]).ptr;

        for (size_t u = 0; u < len; u += sz)
        {   size_t o = u * sz;
            memcpy(tmp, p1 + o, sz);
            memcpy(p1 + o, p2 + o, sz);
            memcpy(p2 + o, tmp, sz);
        }
        if (pbuffer)
            delete pbuffer;
    }

    override void[] init() { return value.init(); }
    override TypeInfo next() { return value; }
    override uint flags() { return value.flags(); }

    TypeInfo value;
    size_t   len;
}

class TypeInfo_AssociativeArray : TypeInfo
{
    override char[] toString()
    {
        return next.toString() ~ "[" ~ key.toString() ~ "]";
    }

    override equals_t opEquals(Object o)
    {
        TypeInfo_AssociativeArray c;
        return this is o ||
                ((c = cast(TypeInfo_AssociativeArray)o) !is null &&
                 this.key == c.key &&
                 this.next == c.next);
    }

    override hash_t getHash(in void* p)
    {
        size_t sz = value.tsize();
        hash_t hash = sz;
        AA aa=*cast(AA*)p;
        size_t keysize=key.tsize();
        int res=_aaApply2(aa, keysize, cast(dg2_t) delegate int(void *k, void *v){
            hash+=rt_hash_combine(key.getHash(k),value.getHash(v));
            return 0;
        });
        return hash;
    }

    override size_t tsize()
    {
        return (char[int]).sizeof;
    }
    override equals_t equals(in void* p1, in void* p2)
    {
        AA a=*cast(AA*)p1;
        AA b=*cast(AA*)p2;
        if (cast(void*)a.a==cast(void*)b.a) return true;
        size_t l1=_aaLen(a);
        size_t l2=_aaLen(b);
        if (l1!=l2) return false;
        size_t keysize=key.tsize();
        equals_t same=true;
        int res=_aaApply2(a, keysize, cast(dg2_t) delegate int(void *k, void *v){
            void* v2=_aaGetRvalue(b, key, value.tsize(), k);
            if (v2 is null || !value.equals(v,v2)) {
                same=false;
                return 1;
            }
            ++l1;
            return 0;
        });
        return same;
    }

    override int compare(in void* p1, in void* p2)
    {
        throw new Exception("non comparable",__FILE__,__LINE__);
    }

    override TypeInfo next() { return value; }
    override uint flags() { return 1; }

    TypeInfo value;
    TypeInfo key;
}

class TypeInfo_Function : TypeInfo
{
    override char[] toString()
    {
        return next.toString() ~ "()";
    }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Function c;
        return this is o ||
                ((c = cast(TypeInfo_Function)o) !is null &&
                 this.next == c.next);
    }

    // BUG: need to add the rest of the functions

    override size_t tsize()
    {
        return 0;       // no size for functions
    }

    TypeInfo next;
}

class TypeInfo_Delegate : TypeInfo
{
    override char[] toString()
    {
        return (next.toString() ~ " delegate()");
    }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Delegate c;
        return this is o ||
                ((c = cast(TypeInfo_Delegate)o) !is null &&
                 this.next == c.next);
    }

    override hash_t getHash(in void* p)
    {
        alias int delegate() dg;
        return rt_hash_str(p,dg.sizeof,0);
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        alias int delegate() dg;
        return memcmp(p1,p2,dg.sizeof);
    }

    override size_t tsize()
    {
        alias int delegate() dg;
        return dg.sizeof;
    }

    override int compare(in void* p1, in void* p2)
    {
        alias int delegate() dg;
        return memcmp(p1,p2,dg.sizeof);
    }

    override uint flags() { return 1; }

    TypeInfo next;
}

class TypeInfo_Class : TypeInfo
{
    override char[] toString() { return info.name; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Class c;
        return this is o ||
                ((c = cast(TypeInfo_Class)o) !is null &&
                 this.info.name == c.classinfo.name);
    }

    override hash_t getHash(in void* p)
    {
        Object o = *cast(Object*)p;
        return o ? o.toHash() : 0;
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        Object o1 = *cast(Object*)p1;
        Object o2 = *cast(Object*)p2;

        return (o1 is o2) || (o1 && o1.opEquals(o2));
    }

    override int compare(in void* p1, in void* p2)
    {
        Object o1 = *cast(Object*)p1;
        Object o2 = *cast(Object*)p2;
        int c = 0;

        // Regard null references as always being "less than"
        if (o1 !is o2)
        {
            if (o1)
            {
                if (!o2)
                    c = 1;
                else
                    c = o1.opCmp(o2);
            }
            else
                c = -1;
        }
        return c;
    }

    override size_t tsize()
    {
        return Object.sizeof;
    }

    override uint flags() { return 1; }

    override OffsetTypeInfo[] offTi()
    {
        return (info.flags & 4) ? info.offTi : null;
    }

    ClassInfo info;
}

class TypeInfo_Interface : TypeInfo
{
    override char[] toString() { return info.name; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Interface c;
        return this is o ||
                ((c = cast(TypeInfo_Interface)o) !is null &&
                 this.info.name == c.classinfo.name);
    }

    override hash_t getHash(in void* p)
    {
        Interface* pi = **cast(Interface ***)*cast(void**)p;
        Object o = cast(Object)(*cast(void**)p - pi.offset);
        assert(o);
        return o.toHash();
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        Interface* pi = **cast(Interface ***)*cast(void**)p1;
        Object o1 = cast(Object)(*cast(void**)p1 - pi.offset);
        pi = **cast(Interface ***)*cast(void**)p2;
        Object o2 = cast(Object)(*cast(void**)p2 - pi.offset);

        return o1 == o2 || (o1 && o1.opCmp(o2) == 0);
    }

    override int compare(in void* p1, in void* p2)
    {
        Interface* pi = **cast(Interface ***)*cast(void**)p1;
        Object o1 = cast(Object)(*cast(void**)p1 - pi.offset);
        pi = **cast(Interface ***)*cast(void**)p2;
        Object o2 = cast(Object)(*cast(void**)p2 - pi.offset);
        int c = 0;

        // Regard null references as always being "less than"
        if (o1 != o2)
        {
            if (o1)
            {
                if (!o2)
                    c = 1;
                else
                    c = o1.opCmp(o2);
            }
            else
                c = -1;
        }
        return c;
    }

    override size_t tsize()
    {
        return Object.sizeof;
    }

    override uint flags() { return 1; }

    ClassInfo info;
}

class TypeInfo_Struct : TypeInfo
{
    override char[] toString() { return name; }

    override equals_t opEquals(Object o)
    {
        TypeInfo_Struct s;
        return this is o ||
                ((s = cast(TypeInfo_Struct)o) !is null &&
                 this.name == s.name &&
                 this.init.length == s.init.length);
    }

    override hash_t getHash(in void* p)
    {
        assert(p);
        if (xtoHash !is null)
        {
            debug(PRINTF) printf("getHash() using xtoHash\n");
            hash_t delegate() toHash;
            toHash.ptr = p;
            toHash.funcptr = cast(hash_t function())xtoHash;
            return toHash();
        }
        else
        {
            debug(PRINTF) printf("getHash() using default hash\n");
            // BUG: relies on the GC not moving objects
            return rt_hash_str(p,init.length,0);
        }
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        if (p1 == p2)
            return true;
        else if (!p1 || !p2)
            return false;
        else if (xopEquals !is null) {
            int delegate(void*) opEquals;
            opEquals.ptr = p1;
            opEquals.funcptr = xopEquals;
            return opEquals(p2);
        } else
            // BUG: relies on the GC not moving objects
            return memcmp(p1, p2, init.length) == 0;
    }

    override int compare(in void* p1, in void* p2)
    {
        // Regard null references as always being "less than"
        if (p1 != p2)
        {
            if (p1)
            {
                if (!p2)
                    return true;
                else if (xopCmp !is null) {
                    int delegate(void*) opCmp;
                    opCmp.ptr = p1;
                    opCmp.funcptr = xopCmp;
                    return opCmp(p2);
                } else
                    // BUG: relies on the GC not moving objects
                    return memcmp(p1, p2, init.length);
            }
            else
                return -1;
        }
        return 0;
    }

    override size_t tsize()
    {
        return init.length;
    }

    override void[] init() { return m_init; }

    override uint flags() { return m_flags; }

    char[] name;
    void[] m_init;      // initializer; init.ptr == null if 0 initialize

    // These are ONLY for use as a delegate.funcptr!
    hash_t function()   xtoHash;
    int function(void*) xopEquals;
    int function(void*) xopCmp;
    char[] function()   xtoString;

    uint m_flags;
}

class TypeInfo_Tuple : TypeInfo
{
    TypeInfo[] elements;

    override char[] toString()
    {
        char[] s;
        s = "(";
        foreach (i, element; elements)
        {
            if (i)
                s ~= ',';
            s ~= element.toString();
        }
        s ~= ")";
        return s;
    }

    override equals_t opEquals(Object o)
    {
        if (this is o)
            return true;

        auto t = cast(TypeInfo_Tuple)o;
        if (t && elements.length == t.elements.length)
        {
            for (size_t i = 0; i < elements.length; i++)
            {
                if (elements[i] != t.elements[i])
                    return false;
            }
            return true;
        }
        return false;
    }

    override hash_t getHash(in void* p)
    {
        assert(0);
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        assert(0);
    }

    override int compare(in void* p1, in void* p2)
    {
        assert(0);
    }

    override size_t tsize()
    {
        assert(0);
    }

    override void swap(void* p1, void* p2)
    {
        assert(0);
    }
}


////////////////////////////////////////////////////////////////////////////////
// Exception
////////////////////////////////////////////////////////////////////////////////
class Exception : Object
{
    struct FrameInfo{
        long line;
        size_t iframe;
        ptrdiff_t offsetSymb;
        size_t baseSymb;
        ptrdiff_t offsetImg;
        size_t baseImg;
        size_t address;
        char[] file;
        char[] func;
        char[] extra;
        bool exactAddress;
        bool internalFunction;
        alias void function(FrameInfo*,void delegate(char[])) FramePrintHandler;
        static FramePrintHandler defaultFramePrintingFunction;
        void writeOut(void delegate(char[])sink){
            if (defaultFramePrintingFunction){
                defaultFramePrintingFunction(this,sink);
            } else {
                char[26] buf;
                auto len=snprintf(buf.ptr,26,"[%8zx]",address);
                sink(buf[0..len]);
                len=snprintf(buf.ptr,26,"%8zx",baseImg);
                sink(buf[0..len]);
                len=snprintf(buf.ptr,26,"%+td ",offsetImg);
                sink(buf[0..len]);
                while (++len<6) sink(" ");
                if (func.length) {
                    sink(func);
                } else {
                    sink("???");
                }
                for (size_t i=func.length;i<80;++i) sink(" ");
                len=snprintf(buf.ptr,26," @%zx",baseSymb);
                sink(buf[0..len]);
                len=snprintf(buf.ptr,26,"%+td ",offsetSymb);
                sink(buf[0..len]);
                if (extra.length){
                    sink(extra);
                    sink(" ");
                }
                sink(file);
                len=snprintf(buf.ptr,26,":%ld ",line);
                sink(buf[0..len]);
            }
        }
        
        void clear(){
            line=0;
            iframe=-1;
            offsetImg=0;
            baseImg=0;
            offsetSymb=0;
            baseSymb=0;
            address=0;
            exactAddress=true;
            internalFunction=false;
            file=null;
            func=null;
            extra=null;
        }
    }
    interface TraceInfo
    {
        int opApply( int delegate( ref FrameInfo fInfo ) );
        void writeOut(void delegate(char[])sink);
    }

    char[]      msg;
    char[]      file;
    size_t      line;  // long would be better
    TraceInfo   info;
    Exception   next;

    this( char[] msg, char[] file, long line, Exception next, TraceInfo info )
    {
        // main constructor, breakpoint this if you want...
        this.msg = msg;
        this.next = next;
        this.file = file;
        this.line = cast(size_t)line;
        this.info = info;
    }

    this( char[] msg, Exception next=null )
    {
        this(msg,"",0,next,rt_createTraceContext(null));
    }

    this( char[] msg, char[] file, long line, Exception next=null )
    {
        this(msg,file,line,next,rt_createTraceContext(null));
    }

    override char[] toString()
    {
        return msg;
    }
    void writeOutMsg(void delegate(char[])sink){
        sink(toString());
    }
    void writeOut(void delegate(char[])sink){
        if (file.length>0 || line!=0)
        {
            char[25]buf;
            sink(this.classinfo.name);
            sink("@");
            sink(file);
            sink("(");
            auto len=snprintf(buf.ptr,26,"%ld",line);
            sink(buf[0..len]);
            sink("): ");
            writeOutMsg(sink);
            sink("\n");
        }
        else
        {
           sink(this.classinfo.name);
           sink(": ");
           writeOutMsg(sink);
           sink("\n");
        }
        if (info)
        {
            sink("----------------\n");
            info.writeOut(sink);
        }
        if (next){
            sink("\n++++++++++++++++\n");
            next.writeOut(sink);
        }
    }
}


alias Exception.TraceInfo function( void* ptr = null ) TraceHandler;
private TraceHandler traceHandler = null;


/**
 * Overrides the default trace hander with a user-supplied version.
 *
 * Params:
 *  h = The new trace handler.  Set to null to use the default handler.
 */
extern (C) void  rt_setTraceHandler( TraceHandler h )
{
    traceHandler = h;
}

/**
 * This function will be called when an Exception is constructed.  The
 * user-supplied trace handler will be called if one has been supplied,
 * otherwise no trace will be generated.
 *
 * Params:
 *  ptr = A pointer to the location from which to generate the trace, or null
 *        if the trace should be generated from within the trace handler
 *        itself.
 *
 * Returns:
 *  An object describing the current calling context or null if no handler is
 *  supplied.
 */
extern(C) Exception.TraceInfo rt_createTraceContext( void* ptr ){
    if( traceHandler is null )
        return null;
    return traceHandler( ptr );
}

////////////////////////////////////////////////////////////////////////////////
// ModuleInfo
////////////////////////////////////////////////////////////////////////////////


enum
{
    MIctorstart  = 1,   // we've started constructing it
    MIctordone   = 2,   // finished construction
    MIstandalone = 4,   // module ctor does not depend on other module
                        // ctors being done first
    MIhasictor   = 8,   // has ictor member
}


class ModuleInfo
{
    char[]          name;
    ModuleInfo[]    importedModules;
    ClassInfo[]     localClasses;
    uint            flags;

    void function() ctor;       // module static constructor (order dependent)
    void function() dtor;       // module static destructor
    void function() unitTest;   // module unit tests

    void* xgetMembers;          // module getMembers() function

    void function() ictor;      // module static constructor (order independent)

    static int opApply( int delegate( ref ModuleInfo ) dg )
    {
        int ret = 0;

        foreach( m; _moduleinfo_array )
        {
            ret = dg( m );
            if( ret )
                break;
        }
        return ret;
    }
}


// Win32: this gets initialized by minit.asm
// linux: this gets initialized in _moduleCtor()
extern (C) ModuleInfo[] _moduleinfo_array;

version (Escape)
{
    // This linked list is created by a compiler generated function inserted
    // into the .ctor list by the compiler.
    struct ModuleReference
    {
        ModuleReference* next;
        ModuleInfo       mod;
    }

    extern (C) ModuleReference* _Dmodule_ref;   // start of linked list
}
else version (linux)
{
    // This linked list is created by a compiler generated function inserted
    // into the .ctor list by the compiler.
    struct ModuleReference
    {
        ModuleReference* next;
        ModuleInfo       mod;
    }

    extern (C) ModuleReference* _Dmodule_ref;   // start of linked list
}

version( freebsd )
{
    // This linked list is created by a compiler generated function inserted 
    // into the .ctor list by the compiler. 
    struct ModuleReference
    { 
        ModuleReference* next; 
        ModuleInfo mod; 
     }
    extern (C) ModuleReference *_Dmodule_ref;   // start of linked list 
}

version( OSX )
{
    extern (C)
    {
    extern void* _minfo_beg;
    extern void* _minfo_end;
    }
}

ModuleInfo[] _moduleinfo_dtors;
uint         _moduleinfo_dtors_i;

// Register termination function pointers
extern (C) int _fatexit(void *);

/**
 * Initialize the modules.
 */

extern (C) void _moduleCtor()
{
    debug(PRINTF) printf("_moduleCtor()\n");
    
    version (Escape)
    {
        int len = 0;
        ModuleReference *mr;

        for (mr = _Dmodule_ref; mr; mr = mr.next)
            len++;
        _moduleinfo_array = new ModuleInfo[len];
        len = 0;
        for (mr = _Dmodule_ref; mr; mr = mr.next)
        {   _moduleinfo_array[len] = mr.mod;
            len++;
        }
    }
    else version (linux)
    {
        int len = 0;
        ModuleReference *mr;

        for (mr = _Dmodule_ref; mr; mr = mr.next)
            len++;
        _moduleinfo_array = new ModuleInfo[len];
        len = 0;
        for (mr = _Dmodule_ref; mr; mr = mr.next)
        {   _moduleinfo_array[len] = mr.mod;
            len++;
        }
    }

    version( freebsd )
    {
        int len = 0;
        ModuleReference *mr;

        for (mr = _Dmodule_ref; mr; mr = mr.next)
            len++;
        _moduleinfo_array = new ModuleInfo[len];
        len = 0;
        for (mr = _Dmodule_ref; mr; mr = mr.next)
        {
            _moduleinfo_array[len] = mr.mod;
            len++;
        }
    }

    version( OSX )
    {
    /* The ModuleInfo references are stored in the special segment
     * __minfodata, which is bracketed by the segments __minfo_beg
     * and __minfo_end. The variables _minfo_beg and _minfo_end
     * are of zero size and are in the two bracketing segments,
     * respectively.
     */
        size_t length = cast(ModuleInfo*)&_minfo_end - cast(ModuleInfo*)&_minfo_beg;
        _moduleinfo_array = (cast(ModuleInfo*)&_minfo_beg)[0 .. length];
        debug(PRINTF) printf("moduleinfo: ptr = %p, length = %d\n", _moduleinfo_array.ptr, _moduleinfo_array.length);

        debug(PRINTF) foreach (m; _moduleinfo_array)
        {
            //printf("\t%p\n", m);
            printf("\t%.*s\n", m.name.length,m.name.ptr);
        }
    }
    
    version (Win32)
    {
        // Ensure module destructors also get called on program termination
        //_fatexit(&_STD_moduleDtor);
    }

    _moduleinfo_dtors = new ModuleInfo[_moduleinfo_array.length];
    debug(PRINTF) printf("_moduleinfo_dtors = x%x\n", cast(void*)_moduleinfo_dtors);
    _moduleIndependentCtors();
    _moduleCtor2(null,_moduleinfo_array, 0);
}

extern (C) void _moduleIndependentCtors()
{
    debug(PRINTF) printf("_moduleIndependentCtors()\n");
    foreach (m; _moduleinfo_array)
    {
        if (m && m.flags & MIhasictor && m.ictor)
        {
            (*m.ictor)();
        }
    }
}

void _moduleCtor2(ModuleInfo from,ModuleInfo[] mi, int skip)
{
    debug(PRINTF) printf("_moduleCtor2(): %d modules\n", mi.length);
    for (uint i = 0; i < mi.length; i++)
    {
        ModuleInfo m = mi[i];

        debug(PRINTF) printf("\tmodule[%d] = '%p'\n", i, m);
        if (!m)
            continue;
        debug(PRINTF) printf("\tmodule[%d] = '%.*s'\n", i, m.name);
        if (m.flags & MIctordone)
            continue;
        debug(PRINTF) printf("\tmodule[%d] = '%.*s', m = x%x\n", i, m.name, m);

        if (m.ctor || m.dtor)
        {
            if (m.flags & MIctorstart)
            {   if (skip || m.flags & MIstandalone)
                    continue;
                throw new Exception( "Cyclic dependency in module " ~ (from is null ? "*null*" : from.name) ~ " for import " ~ m.name); 
            }

            m.flags |= MIctorstart;
            _moduleCtor2(m,m.importedModules, 0);
            if (m.ctor)
                (*m.ctor)();
            m.flags &= ~MIctorstart;
            m.flags |= MIctordone;

            // Now that construction is done, register the destructor
            //printf("\tadding module dtor x%x\n", m);
            assert(_moduleinfo_dtors_i < _moduleinfo_dtors.length);
            _moduleinfo_dtors[_moduleinfo_dtors_i++] = m;
        }
        else
        {
            m.flags |= MIctordone;
            _moduleCtor2(m,m.importedModules, 1);
        }
    }
}

/**
 * Destruct the modules.
 */

// Starting the name with "_STD" means under linux a pointer to the
// function gets put in the .dtors segment.

extern (C) void _moduleDtor()
{
    debug(PRINTF) printf("_moduleDtor(): %d modules\n", _moduleinfo_dtors_i);

    for (uint i = _moduleinfo_dtors_i; i-- != 0;)
    {
        ModuleInfo m = _moduleinfo_dtors[i];

        debug(PRINTF) printf("\tmodule[%d] = '%.*s', x%x\n", i, m.name, m);
        if (m.dtor)
        {
            (*m.dtor)();
        }
    }
    debug(PRINTF) printf("_moduleDtor() done\n");
}

////////////////////////////////////////////////////////////////////////////////
// Monitor
////////////////////////////////////////////////////////////////////////////////

alias Object.Monitor        IMonitor;
alias void delegate(Object) DEvent;

// NOTE: The dtor callback feature is only supported for monitors that are not
//       supplied by the user.  The assumption is that any object with a user-
//       supplied monitor may have special storage or lifetime requirements and
//       that as a result, storing references to local objects within Monitor
//       may not be safe or desirable.  Thus, devt is only valid if impl is
//       null.
struct Monitor
{
    IMonitor impl;
    /* internal */
    DEvent[] devt;
    /* stuff */
}

Monitor* getMonitor(Object h)
{
    return cast(Monitor*) (cast(void**) h)[1];
}

void setMonitor(Object h, Monitor* m)
{
    (cast(void**) h)[1] = m;
}

extern (C) void _d_monitor_create(Object);
extern (C) void _d_monitor_destroy(Object);
extern (C) void _d_monitor_lock(Object);
extern (C) int  _d_monitor_unlock(Object);

extern (C) void _d_monitordelete(Object h, bool det)
{
    Monitor* m = getMonitor(h);

    if (m !is null)
    {
        IMonitor i = m.impl;
        if (i is null)
        {
            _d_monitor_devt(m, h);
            _d_monitor_destroy(h);
            setMonitor(h, null);
            return;
        }
        if (det && (cast(void*) i) !is (cast(void*) h))
            delete i;
        setMonitor(h, null);
    }
}

extern (C) void _d_monitorenter(Object h)
{
    Monitor* m = getMonitor(h);

    if (m is null)
    {
        _d_monitor_create(h);
        m = getMonitor(h);
    }

    IMonitor i = m.impl;

    if (i is null)
    {
        _d_monitor_lock(h);
        return;
    }
    i.lock();
}

extern (C) void _d_monitorexit(Object h)
{
    Monitor* m = getMonitor(h);
    IMonitor i = m.impl;

    if (i is null)
    {
        _d_monitor_unlock(h);
        return;
    }
    i.unlock();
}

extern (C) void _d_monitor_devt(Monitor* m, Object h)
{
    if (m.devt.length)
    {
        DEvent[] devt;

        synchronized (h)
        {
            devt = m.devt;
            m.devt = null;
        }
        foreach (v; devt)
        {
            if (v)
                v(h);
        }
        free(devt.ptr);
    }
}

extern (C) void rt_attachDisposeEvent(Object h, DEvent e)
{
    synchronized (h)
    {
        Monitor* m = getMonitor(h);
        assert(m.impl is null);

        foreach (ref v; m.devt)
        {
            if (v is null || v == e)
            {
                v = e;
                return;
            }
        }

        auto len = m.devt.length + 4; // grow by 4 elements
        auto pos = m.devt.length;     // insert position
        auto p = realloc(m.devt.ptr, DEvent.sizeof * len);
        if (!p)
            onOutOfMemoryError();
        m.devt = (cast(DEvent*)p)[0 .. len];
        m.devt[pos+1 .. len] = null;
        m.devt[pos] = e;
    }
}

extern (C) bool rt_detachDisposeEvent(Object h, DEvent e)
{
    synchronized (h)
    {
        Monitor* m = getMonitor(h);
        assert(m.impl is null);

        foreach (p, v; m.devt)
        {
            if (v == e)
            {
                memmove(&m.devt[p],
                        &m.devt[p+1],
                        (m.devt.length - p - 1) * DEvent.sizeof);
                m.devt[$ - 1] = null;
                return true;
            }
        }
    }
    return false;
}
