/**
 * The variant module contains a variant, or polymorphic type.
 *
 * Copyright: Copyright (C) 2005-2009 The Tango Team.  All rights reserved.
 * License:   BSD style: $(LICENSE)
 * Authors:   Daniel Keep, Sean Kelly
 */
module tango.core.Variant;

private import tango.core.Memory : GC;
private import tango.core.Vararg : va_list;
private import tango.core.Traits;
private import tango.core.Tuple;

private extern(C) Object _d_toObject(void*);

/*
 * This is to control when we compile in vararg support.  Vararg is a complete
 * pain in the arse.  I haven't been able to test under GDC at all (and
 * support for it may disappear soon anyway) and LDC refuses to build for me.
 *
 * As other compilers are tested and verified to work, they should be added
 * below.  It would also probably be a good idea to verify the platforms for
 * which it works.
 */

version( DigitalMars )
{
    version( X86 )
    {
        version( Windows )
        {
            version=EnableVararg;
        }
        else version( Posix )
        {
            version=EnableVararg;
        }
    }
}
else version( LDC )
{
    version( X86 )
    {
        version( linux )
        {
            version=EnableVararg; // thanks rawler
        }
    }
    else version( X86_64 )
    {
        version( linux )
        {
            version=EnableVararg; // thanks mwarning
        }
    }
}
else version( DDoc )
{
    // Let's hope DDoc is smart enough to catch this...
    version=EnableVararg;
}

version( EnableVararg ) {} else
{
    pragma(msg, "Note: Variant vararg functionality not supported for this "
            "compiler/platform combination.");
    pragma(msg, "To override and enable vararg support anyway, compile with "
            "the EnableVararg version.");
}

private
{
    /*
     * This is used to store the actual value being kept in a Variant.
     */
    struct VariantStorage
    {
        union
        {
            /*
             * Contains heap-allocated storage for values which are too large
             * to fit into the Variant directly.
             */
            void[] heap;

            /*
             * Used to store arrays directly.  Note that this is NOT an actual
             * array; using a void[] causes the length to change, which screws
             * up the ptr() property.
             *
             * WARNING: this structure MUST match the ABI for arrays for this
             * platform.  AFAIK, all compilers implement arrays this way.
             * There needs to be a case in the unit test to ensure this.
             */
            struct Array
            {
                size_t length;
                void* ptr;
            }
            Array array;

            // Used to simplify dealing with objects.
            Object obj;

            // Used to address storage as an array.
            ubyte[array.sizeof] data;
        }

        /*
         * This is used to set the array structure safely.  We're essentially
         * just ensuring that if a garbage collection happens mid-assign, we
         * don't accidentally mark bits of memory we shouldn't.
         *
         * Of course, the compiler could always re-order the length and ptr
         * assignment.  Oh well.
         */
        void setArray(void* ptr, size_t length)
        {
            array.length = 0;
            array.ptr = ptr;
            array.length = length;
        }
    }

    // Determines if the given type is an Object (class) type.
    template isObject(T)
    {
        static if( is( T : Object ) )
            const isObject = true;
        else
            const isObject = false;
    }

    // Determines if the given type is an interface
    template isInterface(T)
    {
        static if( is( T == interface ) )
            const isInterface = true;
        else
            const isInterface = false;
    }

    // A list of all basic types
    alias Tuple!(bool, char, wchar, dchar,
            byte, short, int, long, //cent,
            ubyte, ushort, uint, ulong, //ucent,
            float, double, real,
            ifloat, idouble, ireal,
            cfloat, cdouble, creal) BasicTypes;

    // see isBasicType
    template isBasicTypeImpl(T, U)
    {
        const isBasicTypeImpl = is( T == U );
    }

    // see isBasicType
    template isBasicTypeImpl(T, U, Us...)
    {
        static if( is( T == U ) )
            const isBasicTypeImpl = true;
        else
            const isBasicTypeImpl = isBasicTypeImpl!(T, Us);
    }

    // Determines if the given type is one of the basic types.
    template isBasicType(T)
    {
        const isBasicType = isBasicTypeImpl!(T, BasicTypes);
    }

    /*
     * Used to determine if we can cast a value of the given TypeInfo to the
     * specified type implicitly.  This should be somewhat faster than the
     * version in RuntimeTraits since we can basically eliminate half of the
     * tests.
     */
    bool canImplicitCastToType(dsttypeT)(TypeInfo srctype)
    {
        /*
         * Before we do anything else, we need to "unwrap" typedefs to
         * get at the real type.  While we do that, make sure we don't
         * accidentally jump over the destination type.
         */
        while( cast(TypeInfo_Typedef) srctype !is null )
        {
            if( srctype is typeid(dsttypeT) )
                return true;
            srctype = srctype.next;
        }

        /*
         * First, we'll generate tests for the basic types.  The list of
         * things which can be cast TO basic types is finite and easily
         * computed.
         */
        foreach( T ; BasicTypes )
        {
            // If the current type is the target...
            static if( is( dsttypeT == T ) )
            {
                // ... then for all of the other basic types ...
                foreach( U ; BasicTypes )
                {
                    static if
                    (
                        // ... if that type is smaller than ...
                        U.sizeof < T.sizeof

                        // ... or the same size and signed-ness ...
                        || ( U.sizeof == T.sizeof &&
                            ((isCharType!(T) || isUnsignedIntegerType!(T))
                             ^ !(isCharType!(U) || isUnsignedIntegerType!(U)))
                        )
                    )
                    {
                        // ... test.
                        if( srctype is typeid(U) )
                            return true;
                    }
                }
                // Nothing matched; no implicit casting.
                return false;
            }
        }

        /*
         * Account for static arrays being implicitly convertible to dynamic
         * arrays.
         */
        static if( is( T[] : dsttypeT ) )
        {
            if( typeid(T[]) is srctype )
                return true;

            if( auto ti_sa = cast(TypeInfo_StaticArray) srctype )
                return ti_sa.next is typeid(T);

            return false;
        }

        /*
         * Any pointer can be cast to void*.
         */
        static if( is( dsttypeT == void* ) )
            return (cast(TypeInfo_Pointer) srctype) !is null;

        /*
         * Any array can be cast to void[], however remember that it has to
         * be manually adjusted to preserve the correct length.
         */
        static if( is( dsttypeT == void[] ) )
            return ((cast(TypeInfo_Array) srctype) !is null)
                || ((cast(TypeInfo_StaticArray) srctype) !is null);

        return false;
    }

    /*
     * Aliases itself to the type used to return a value of type T out of a
     * function.  This is basically a work-around for not being able to return
     * static arrays.
     */
    template returnT(T)
    {
        static if( isStaticArrayType!(T) )
            alias typeof(T.dup) returnT;
        else
            alias T returnT;
    }

    /*
     * Here are some tests that perform runtime versions of the compile-time
     * traits functions.
     */

    bool isBasicTypeInfo(TypeInfo ti)
    {
        foreach( T ; BasicTypes )
            if( ti is typeid(T) )
                return true;
        return false;
    }

    private import RuntimeTraits = tango.core.RuntimeTraits;

    alias RuntimeTraits.isStaticArray isStaticArrayTypeInfo;
    alias RuntimeTraits.isClass isObjectTypeInfo;
    alias RuntimeTraits.isInterface isInterfaceTypeInfo;
}

/**
 * This exception is thrown whenever you attempt to get the value of a Variant
 * without using a compatible type.
 */
class VariantTypeMismatchException : Exception
{
    this(TypeInfo expected, TypeInfo got)
    {
        super("cannot convert "~expected.toString
                    ~" value to a "~got.toString);
    }
}

/**
 * This exception is thrown when you attempt to use an empty Variant with
 * varargs.
 */
class VariantVoidVarargException : Exception
{
    this()
    {
        super("cannot use Variants containing a void with varargs");
    }
}

/**
 * The Variant type is used to dynamically store values of different types at
 * runtime.
 *
 * You can create a Variant using either the pseudo-constructor or direct
 * assignment.
 *
 * -----
 *  Variant v = Variant(42);
 *  v = "abc";
 * -----
 */
struct Variant
{
    /**
     * This pseudo-constructor is used to place a value into a new Variant.
     *
     * Params:
     *  value = The value you wish to put in the Variant.
     *
     * Returns:
     *  The new Variant.
     * 
     * Example:
     * -----
     *  auto v = Variant(42);
     * -----
     */
    static Variant opCall(T)(T value)
    {
        Variant _this;

        static if( isStaticArrayType!(T) )
            _this = value.dup;

        else
            _this = value;

        return _this;
    }

    /**
     * This pseudo-constructor creates a new Variant using a specified
     * TypeInfo and raw pointer to the value.
     *
     * Params:
     *  type = Type of the value.
     *  ptr  = Pointer to the value.
     *
     * Returns:
     *  The new Variant.
     * 
     * Example:
     * -----
     *  int life = 42;
     *  auto v = Variant(typeid(typeof(life)), &life);
     * -----
     */
    static Variant opCall()(TypeInfo type, void* ptr)
    {
        Variant _this;
        Variant.fromPtr(type, ptr, _this);
        return _this;
    }

    /**
     * This operator allows you to assign arbitrary values directly into an
     * existing Variant.
     *
     * Params:
     *  value = The value you wish to put in the Variant.
     *
     * Returns:
     *  The new value of the assigned-to variant.
     * 
     * Example:
     * -----
     *  Variant v;
     *  v = 42;
     * -----
     */
    Variant opAssign(T)(T value)
    {
        static if( isStaticArrayType!(T) )
        {
            return (*this = value.dup);
        }
        else
        {
            type = typeid(T);

            static if( isDynamicArrayType!(T) )
            {
                this.value.setArray(value.ptr, value.length);
            }
            else static if( isInterface!(T) )
            {
                this.value.obj = cast(Object) value;
            }
            else
            {
                /*
                 * If the value is small enough to fit in the storage
                 * available, do so.  If it isn't, then make a heap copy.
                 *
                 * Obviously, this pretty clearly breaks value semantics for
                 * large values, but without a postblit operator, there's not
                 * much we can do.  :(
                 */
                static if( T.sizeof <= this.value.data.length )
                {
                    this.value.data[0..T.sizeof] =
                        (cast(ubyte*)&value)[0..T.sizeof];
                }
                else
                {
                    auto buffer = (cast(ubyte*)&value)[0..T.sizeof].dup;
                    this.value.heap = cast(void[])buffer;
                }
            }
            return *this;
        }
    }

    /**
     * This member can be used to determine if the value stored in the Variant
     * is of the specified type.  Note that this comparison is exact: it does
     * not take implicit casting rules into account.
     *
     * Returns:
     *  true if the Variant contains a value of type T, false otherwise.
     * 
     * Example:
     * -----
     *  auto v = Variant(cast(int) 42);
     *  assert(   v.isA!(int) );
     *  assert( ! v.isA!(short) ); // note no implicit conversion
     * -----
     */
    bool isA(T)()
    {
        return cast(bool)(typeid(T) is type);
    }

    /**
     * This member can be used to determine if the value stored in the Variant
     * is of the specified type.  This comparison attempts to take implicit
     * conversion rules into account.
     *
     * Returns:
     *  true if the Variant contains a value of type T, or if the Variant
     *  contains a value that can be implicitly cast to type T; false
     *  otherwise.
     * 
     * Example:
     * -----
     *  auto v = Variant(cast(int) 42);
     *  assert( v.isA!(int) );
     *  assert( v.isA!(short) ); // note implicit conversion
     * -----
     */
    bool isImplicitly(T)()
    {
        static if( is( T == class ) || is( T == interface ) )
        {
            // Check for classes and interfaces first.
            if( cast(TypeInfo_Class) type || cast(TypeInfo_Interface) type )
                return (cast(T) value.obj) !is null;

            else
                // We're trying to cast TO an object, but we don't have
                // an object stored.
                return false;
        }
        else
        {
            // Test for basic types (oh, and dynamic->static arrays and
            // pointers.)
            return ( cast(bool)(typeid(T) is type)
                    || canImplicitCastToType!(T)(type) );
        }
    }

    /**
     * This determines whether the Variant has an assigned value or not.  It
     * is simply short-hand for calling the isA member with a type of void.
     *
     * Returns:
     *  true if the Variant does not contain a value, false otherwise.
     */
    bool isEmpty()
    {
        return isA!(void);
    }

    /**
     * This member will clear the Variant, returning it to an empty state.
     */
    void clear()
    {
        _type = typeid(void);
        value = value.init;
    }

    version( DDoc )
    {
        /**
         * This is the primary mechanism for extracting a value from a Variant.
         * Given a destination type S, it will attempt to extract the value of the
         * Variant into that type.  If the value contained within the Variant
         * cannot be implicitly cast to the given type S, it will throw an
         * exception.
         *
         * You can check to see if this operation will fail by calling the
         * isImplicitly member with the type S.
         *
         * Note that attempting to get a statically-sized array will result in a
         * dynamic array being returned; this is a language limitation.
         *
         * Returns:
         *  The value stored within the Variant.
         */
        T get(T)()
        {
            // For actual implementation, see below.
        }
    }
    else
    {
        returnT!(S) get(S)()
        {
            alias returnT!(S) T;

            // If we're not dealing with the exact same type as is being
            // stored, we fail NOW if the type in question isn't an object (we
            // can let the runtime do the test) and if it isn't something we
            // know we can implicitly cast to.
            if( type !is typeid(T)
                    // Let D do runtime check itself
                    && !isObject!(T)
                    && !isInterface!(T)

                    // Allow implicit upcasts
                    && !canImplicitCastToType!(T)(type)
              )
                throw new VariantTypeMismatchException(type,typeid(T));

            // Handle basic types, since they account for most of the implicit
            // casts.
            static if( isBasicType!(T) )
            {
                if( type is typeid(T) )
                {
                    // We got lucky; the types match exactly.  If the type is
                    // small, grab it out of storage; otherwise, copy it from
                    // the heap.
                    static if( T.sizeof <= value.sizeof )
                        return *cast(T*)(&value);

                    else
                        return *cast(T*)(value.heap.ptr);
                }
                else
                {
                    // This handles implicit coercion.  What it does is finds
                    // the basic type U which is actually being stored.  It
                    // then unpacks the value of type U stored in the Variant
                    // and casts it to type T.
                    //
                    // It is assumed that this is valid to perform since we
                    // should have already eliminated invalid coercions.
                    foreach( U ; BasicTypes )
                    {
                        if( type is typeid(U) )
                        {
                            static if( U.sizeof <= value.sizeof )
                                return cast(T) *cast(U*)(&value);

                            else
                                return cast(T) *cast(U*)(value.heap.ptr);
                        }
                    }
                    throw new VariantTypeMismatchException(type,typeid(T));
                }
            }
            else static if( isDynamicArrayType!(T) )
            {
                return (cast(typeof(T.ptr)) value.array.ptr)
                    [0..value.array.length];
            }
            else static if( isObject!(T) || isInterface!(T) )
            {
                return cast(T)this.value.obj;
            }
            else
            {
                static if( T.sizeof <= this.value.data.length )
                {
                    T result;
                    (cast(ubyte*)&result)[0..T.sizeof] =
                        this.value.data[0..T.sizeof];
                    return result;
                }
                else
                {
                    T result;
                    (cast(ubyte*)&result)[0..T.sizeof] =
                        (cast(ubyte[])this.value.heap)[0..T.sizeof];
                    return result;
                }
            }
        }
    }

    /**
     * The following operator overloads are defined for the sake of
     * convenience.  It is important to understand that they do not allow you
     * to use a Variant as both the left-hand and right-hand sides of an
     * expression.  One side of the operator must be a concrete type in order
     * for the Variant to know what code to generate.
     */
    typeof(T+T) opAdd(T)(T rhs)     { return get!(T) + rhs; }
    typeof(T+T) opAdd_r(T)(T lhs)   { return lhs + get!(T); } /// ditto
    typeof(T-T) opSub(T)(T rhs)     { return get!(T) - rhs; } /// ditto
    typeof(T-T) opSub_r(T)(T lhs)   { return lhs - get!(T); } /// ditto
    typeof(T*T) opMul(T)(T rhs)     { return get!(T) * rhs; } /// ditto
    typeof(T*T) opMul_r(T)(T lhs)   { return lhs * get!(T); } /// ditto
    typeof(T/T) opDiv(T)(T rhs)     { return get!(T) / rhs; } /// ditto
    typeof(T/T) opDiv_r(T)(T lhs)   { return lhs / get!(T); } /// ditto
    typeof(T%T) opMod(T)(T rhs)     { return get!(T) % rhs; } /// ditto
    typeof(T%T) opMod_r(T)(T lhs)   { return lhs % get!(T); } /// ditto
    typeof(T&T) opAnd(T)(T rhs)     { return get!(T) & rhs; } /// ditto
    typeof(T&T) opAnd_r(T)(T lhs)   { return lhs & get!(T); } /// ditto
    typeof(T|T) opOr(T)(T rhs)      { return get!(T) | rhs; } /// ditto
    typeof(T|T) opOr_r(T)(T lhs)    { return lhs | get!(T); } /// ditto
    typeof(T^T) opXor(T)(T rhs)     { return get!(T) ^ rhs; } /// ditto
    typeof(T^T) opXor_r(T)(T lhs)   { return lhs ^ get!(T); } /// ditto
    typeof(T<<T) opShl(T)(T rhs)    { return get!(T) << rhs; } /// ditto
    typeof(T<<T) opShl_r(T)(T lhs)  { return lhs << get!(T); } /// ditto
    typeof(T>>T) opShr(T)(T rhs)    { return get!(T) >> rhs; } /// ditto
    typeof(T>>T) opShr_r(T)(T lhs)  { return lhs >> get!(T); } /// ditto
    typeof(T>>>T) opUShr(T)(T rhs)  { return get!(T) >>> rhs; } /// ditto
    typeof(T>>>T) opUShr_r(T)(T lhs){ return lhs >>> get!(T); } /// ditto
    typeof(T~T) opCat(T)(T rhs)     { return get!(T) ~ rhs; } /// ditto
    typeof(T~T) opCat_r(T)(T lhs)   { return lhs ~ get!(T); } /// ditto

    Variant opAddAssign(T)(T value) { return (*this = get!(T) + value); } /// ditto
    Variant opSubAssign(T)(T value) { return (*this = get!(T) - value); } /// ditto
    Variant opMulAssign(T)(T value) { return (*this = get!(T) * value); } /// ditto
    Variant opDivAssign(T)(T value) { return (*this = get!(T) / value); } /// ditto
    Variant opModAssign(T)(T value) { return (*this = get!(T) % value); } /// ditto
    Variant opAndAssign(T)(T value) { return (*this = get!(T) & value); } /// ditto
    Variant opOrAssign(T)(T value)  { return (*this = get!(T) | value); } /// ditto
    Variant opXorAssign(T)(T value) { return (*this = get!(T) ^ value); } /// ditto
    Variant opShlAssign(T)(T value) { return (*this = get!(T) << value); } /// ditto
    Variant opShrAssign(T)(T value) { return (*this = get!(T) >> value); } /// ditto
    Variant opUShrAssign(T)(T value){ return (*this = get!(T) >>> value); } /// ditto
    Variant opCatAssign(T)(T value) { return (*this = get!(T) ~ value); } /// ditto

    /**
     * The following operators can be used with Variants on both sides.  Note
     * that these operators do not follow the standard rules of
     * implicit conversions.
     */
    int opEquals(T)(T rhs)
    {
        static if( is( T == Variant ) )
            return opEqualsVariant(rhs);

        else
            return get!(T) == rhs;
    }

    /// ditto
    int opCmp(T)(T rhs)
    {
        static if( is( T == Variant ) )
            return opCmpVariant(rhs);
        else
        {
            auto lhs = get!(T);
            return (lhs < rhs) ? -1 : (lhs == rhs) ? 0 : 1;
        }
    }

    /// ditto
    hash_t toHash()
    {
        return type.getHash(this.ptr);
    }

    /**
     * Returns a string representation of the type being stored in this
     * Variant.
     *
     * Returns:
     *  The string representation of the type contained within the Variant.
     */
    char[] toString()
    {
        return type.toString;
    }

    /**
     * This can be used to retrieve the TypeInfo for the currently stored
     * value.
     */
    TypeInfo type()
    {
        return _type;
    }

    /**
     * This can be used to retrieve a pointer to the value stored in the
     * variant.
     */
    void* ptr()
    {
        if( type.tsize <= value.sizeof )
            return &value;

        else
            return value.heap.ptr;
    }
    
    version( EnableVararg )
    {
        /**
         * Converts a vararg function argument list into an array of Variants.
         */
        static Variant[] fromVararg(TypeInfo[] types, va_list args)
        {
            auto vs = new Variant[](types.length);

            foreach( i, ref v ; vs )
                args = Variant.fromPtr(types[i], args, v);
            
            return vs;
        }
        
        /// ditto
        static Variant[] fromVararg(...)
        {
            return Variant.fromVararg(_arguments, _argptr);
        }
        
        /**
         * Converts an array of Variants into a vararg function argument list.
         *
         * This will allocate memory to store the arguments in; you may destroy
         * this memory when you are done with it if you feel so inclined.
         */
        static void toVararg(Variant[] vars, out TypeInfo[] types, out va_list args)
        {
            // First up, compute the total amount of space we'll need.  While
            // we're at it, work out if any of the values we're storing have
            // pointers.  If they do, we'll need to tell the GC.
            size_t size = 0;
            bool noptr = true;
            foreach( ref v ; vars )
            {
                auto ti = v.type;
                size += (ti.tsize + size_t.sizeof-1) & ~(size_t.sizeof-1);
                noptr = noptr && (ti.flags & 2);
            }
            
            // Create the storage, and tell the GC whether it needs to be scanned
            // or not.
            auto storage = new ubyte[size];
            GC.setAttr(storage.ptr,
                (GC.getAttr(storage.ptr) & ~GC.BlkAttr.NO_SCAN)
                | (noptr ? GC.BlkAttr.NO_SCAN : 0));

            // Dump the variants into the storage.
            args = storage.ptr;
            auto arg_temp = args;

            types = new TypeInfo[vars.length];

            foreach( i, ref v ; vars )
            {
                types[i] = v.type;
                arg_temp = v.toPtr(arg_temp);
            }
        }
    } // version( EnableVararg )

private:
    TypeInfo _type = typeid(void);
    VariantStorage value;

    TypeInfo type(TypeInfo v)
    {
        return (_type = v);
    }
    
    /*
     * Creates a Variant using a given TypeInfo and a void*.  Returns the
     * given pointer adjusted for the next vararg.
     */
    static void* fromPtr(TypeInfo type, void* ptr, out Variant r)
    {
        /*
         * This function basically duplicates the functionality of
         * opAssign, except that we can't generate code based on the
         * type of the data we're storing.
         */

        if( type is typeid(void) )
            throw new VariantVoidVarargException;

        r.type = type;

        if( isStaticArrayTypeInfo(type) )
        {
            /*
             * Static arrays are passed by-value; for example, if type is
             * typeid(int[4]), then ptr is a pointer to 16 bytes of memory
             * (four 32-bit integers).
             *
             * It's possible that the memory being pointed to is on the
             * stack, so we need to copy it before storing it.  type.tsize
             * tells us exactly how many bytes we need to copy.
             *
             * Sadly, we can't directly construct the dynamic array version
             * of type.  We'll store the static array type and cope with it
             * in isImplicitly(S) and get(S).
             */
            r.value.heap = ptr[0 .. type.tsize].dup;
        }
        else
        {
            if( isObjectTypeInfo(type)
                || isInterfaceTypeInfo(type) )
            {
                /*
                 * We have to call into the core runtime to turn this pointer
                 * into an actual Object reference.
                 */
                r.value.obj = _d_toObject(*cast(void**)ptr);
            }
            else
            {
                if( type.tsize <= this.value.data.length )
                {
                    // Copy into storage
                    r.value.data[0 .. type.tsize] = 
                        (cast(ubyte*)ptr)[0 .. type.tsize];
                }
                else
                {
                    // Store in heap
                    auto buffer = (cast(ubyte*)ptr)[0 .. type.tsize].dup;
                    r.value.heap = cast(void[])buffer;
                }
            }
        }

        // Compute the "advanced" pointer.
        return ptr + ( (type.tsize + size_t.sizeof-1) & ~(size_t.sizeof-1) );
    }

    version( EnableVararg )
    {
        /*
         * Takes the current Variant, and dumps its contents into memory pointed
         * at by a void*, suitable for vararg calls.
         *
         * It also returns the supplied pointer adjusted by the size of the data
         * written to memory.
         */
        void* toPtr(void* ptr)
        {
            version( GNU )
            {
                pragma(msg, "WARNING: tango.core.Variant's vararg support has "
                        "not been tested with this compiler." );
            }
            version( LDC )
            {
                pragma(msg, "WARNING: tango.core.Variant's vararg support has "
                        "not been tested with this compiler." );
            }

            if( type is typeid(void) )
                throw new VariantVoidVarargException;

            if( isStaticArrayTypeInfo(type) )
            {
                // Just dump straight
                ptr[0 .. type.tsize] = this.value.heap[0 .. type.tsize];
            }
            else
            {
                if( isInterfaceTypeInfo(type) )
                {
                    /*
                     * This is tricky.  What we actually have stored in
                     * value.obj is an Object, not an interface.  What we
                     * need to do is manually "cast" value.obj to the correct
                     * interface.
                     *
                     * We have the original interface's TypeInfo.  This gives us
                     * the interface's ClassInfo.  We can also obtain the object's
                     * ClassInfo which contains a list of Interfaces.
                     *
                     * So what we need to do is loop over the interfaces obj
                     * implements until we find the one we're interested in.  Then
                     * we just read out the interface's offset and adjust obj
                     * accordingly.
                     */
                    auto type_i = cast(TypeInfo_Interface) type;
                    bool found = false;
                    foreach( i ; this.value.obj.classinfo.interfaces )
                    {
                        if( i.classinfo is type_i.info )
                        {
                            // Found it
                            void* i_ptr = (cast(void*) this.value.obj) + i.offset;
                            *cast(void**)ptr = i_ptr;
                            found = true;
                            break;
                        }
                    }
                    assert(found,"Could not convert Object to interface; "
                            "bad things have happened.");
                }
                else
                {
                    if( type.tsize <= this.value.data.length )
                    {
                        // Value stored in storage
                        ptr[0 .. type.tsize] = this.value.data[0 .. type.tsize];
                    }
                    else
                    {
                        // Value stored on heap
                        ptr[0 .. type.tsize] = this.value.heap[0 .. type.tsize];
                    }
                }
            }

            // Compute the "advanced" pointer.
            return ptr + ( (type.tsize + size_t.sizeof-1) & ~(size_t.sizeof-1) );
        }
    } // version( EnableVararg )

    /*
     * Performs a type-dependant comparison.  Note that this obviously doesn't
     * take into account things like implicit conversions.
     */
    int opEqualsVariant(Variant rhs)
    {
        if( type != rhs.type ) return false;
        return cast(bool) type.equals(this.ptr, rhs.ptr);
    }

    /*
     * Same as opEqualsVariant except it does opCmp.
     */
    int opCmpVariant(Variant rhs)
    {
        if( type != rhs.type )
            throw new VariantTypeMismatchException(type, rhs.type);
        return type.compare(this.ptr, rhs.ptr);
    }
}

debug( UnitTest )
{
    /*
     * Language tests.
     */

    unittest
    {
        {
            int[2] a;
            void[] b = a;
            int[]  c = cast(int[]) b;
            assert( b.length == 2*int.sizeof );
            assert( c.length == a.length );
        }

        {
            struct A { size_t l; void* p; }
            char[] b = "123";
            A a = *cast(A*)(&b);

            assert( a.l == b.length );
            assert( a.p == b.ptr );
        }
    }

    /*
     * Basic tests.
     */

    unittest
    {
        Variant v;
        assert( v.isA!(void), v.type.toString );
        assert( v.isEmpty, v.type.toString );

        // Test basic integer storage and implicit casting support
        v = 42;
        assert( v.isA!(int), v.type.toString );
        assert( v.isImplicitly!(long), v.type.toString );
        assert( v.isImplicitly!(ulong), v.type.toString );
        assert( !v.isImplicitly!(uint), v.type.toString );
        assert( v.get!(int) == 42 );
        assert( v.get!(long) == 42L );
        assert( v.get!(ulong) == 42uL );

        // Test clearing
        v.clear;
        assert( v.isA!(void), v.type.toString );
        assert( v.isEmpty, v.type.toString );

        // Test strings
        v = "Hello, World!"c;
        assert( v.isA!(char[]), v.type.toString );
        assert( !v.isImplicitly!(wchar[]), v.type.toString );
        assert( v.get!(char[]) == "Hello, World!" );

        // Test array storage
        v = [1,2,3,4,5];
        assert( v.isA!(int[]), v.type.toString );
        assert( v.get!(int[]) == [1,2,3,4,5] );

        // Make sure arrays are correctly stored so that .ptr works.
        {
            int[] a = [1,2,3,4,5];
            v = a;
            auto b = *cast(int[]*)(v.ptr);

            assert( a.ptr == b.ptr );
            assert( a.length == b.length );
        }

        // Test pointer storage
        v = &v;
        assert( v.isA!(Variant*), v.type.toString );
        assert( !v.isImplicitly!(int*), v.type.toString );
        assert( v.isImplicitly!(void*), v.type.toString );
        assert( v.get!(Variant*) == &v );

        // Test object storage
        {
            scope o = new Object;
            v = o;
            assert( v.isA!(Object), v.type.toString );
            assert( v.get!(Object) is o );
        }

        // Test interface support
        {
            interface A {}
            interface B : A {}
            class C : B {}
            class D : C {}

            A a = new D;
            Variant v2 = a;
            B b = v2.get!(B);
            C c = v2.get!(C);
            D d = v2.get!(D);
        }

        // Test class/interface implicit casting
        {
            class G {}
            interface H {}
            class I : G {}
            class J : H {}
            struct K {}

            scope a = new G;
            scope c = new I;
            scope d = new J;
            K e;

            Variant v2 = a;
            assert( v2.isImplicitly!(Object), v2.type.toString );
            assert( v2.isImplicitly!(G), v2.type.toString );
            assert(!v2.isImplicitly!(I), v2.type.toString );

            v2 = c;
            assert( v2.isImplicitly!(Object), v2.type.toString );
            assert( v2.isImplicitly!(G), v2.type.toString );
            assert( v2.isImplicitly!(I), v2.type.toString );

            v2 = d;
            assert( v2.isImplicitly!(Object), v2.type.toString );
            assert(!v2.isImplicitly!(G), v2.type.toString );
            assert( v2.isImplicitly!(H), v2.type.toString );
            assert( v2.isImplicitly!(J), v2.type.toString );

            v2 = e;
            assert(!v2.isImplicitly!(Object), v2.type.toString );
        }

        // Test doubles and implicit casting
        v = 3.1413;
        assert( v.isA!(double), v.type.toString );
        assert( v.isImplicitly!(real), v.type.toString );
        assert( !v.isImplicitly!(float), v.type.toString );
        assert( v.get!(double) == 3.1413 );

        // Test storage transitivity
        auto u = Variant(v);
        assert( u.isA!(double), u.type.toString );
        assert( u.get!(double) == 3.1413 );

        // Test operators
        v = 38;
        assert( v + 4 == 42 );
        assert( 4 + v == 42 );
        assert( v - 4 == 34 );
        assert( 4 - v == -34 );
        assert( v * 2 == 76 );
        assert( 2 * v == 76 );
        assert( v / 2 == 19 );
        assert( 2 / v == 0 );
        assert( v % 2 == 0 );
        assert( 2 % v == 2 );
        assert( (v & 6) == 6 );
        assert( (6 & v) == 6 );
        assert( (v | 9) == 47 );
        assert( (9 | v) == 47 );
        assert( (v ^ 5) == 35 );
        assert( (5 ^ v) == 35 );
        assert( v << 1 == 76 );
        assert( 1 << Variant(2) == 4 );
        assert( v >> 1 == 19 );
        assert( 4 >> Variant(2) == 1 );

        assert( Variant("abc") ~ "def" == "abcdef" );
        assert( "abc" ~ Variant("def") == "abcdef" );

        // Test op= operators
        v = 38; v += 4; assert( v == 42 );
        v = 38; v -= 4; assert( v == 34 );
        v = 38; v *= 2; assert( v == 76 );
        v = 38; v /= 2; assert( v == 19 );
        v = 38; v %= 2; assert( v == 0 );
        v = 38; v &= 6; assert( v == 6 );
        v = 38; v |= 9; assert( v == 47 );
        v = 38; v ^= 5; assert( v == 35 );
        v = 38; v <<= 1; assert( v == 76 );
        v = 38; v >>= 1; assert( v == 19 );

        v = "abc"; v ~= "def"; assert( v == "abcdef" );

        // Test comparison
        assert( Variant(0) < Variant(42) );
        assert( Variant(42) > Variant(0) );
        assert( Variant(21) == Variant(21) );
        assert( Variant(0) != Variant(42) );
        assert( Variant("bar") == Variant("bar") );
        assert( Variant("foo") != Variant("bar") );

        // Test variants as AA keys
        {
            auto v1 = Variant(42);
            auto v2 = Variant("foo");
            auto v3 = Variant(1+2.0i);

            int[Variant] hash;
            hash[v1] = 0;
            hash[v2] = 1;
            hash[v3] = 2;

            assert( hash[v1] == 0 );
            assert( hash[v2] == 1 );
            assert( hash[v3] == 2 );
        }

        // Test AA storage
        {
            int[char[]] hash;
            hash["a"] = 1;
            hash["b"] = 2;
            hash["c"] = 3;
            Variant vhash = hash;

            assert( vhash.get!(int[char[]])["a"] == 1 );
            assert( vhash.get!(int[char[]])["b"] == 2 );
            assert( vhash.get!(int[char[]])["c"] == 3 );
        }
    }

    /*
     * Vararg tests.
     */

    version( EnableVararg )
    {
        private import tango.core.Vararg;

        unittest
        {
            class A
            {
                char[] msg() { return "A"; }
            }
            class B : A
            {
                override char[] msg() { return "B"; }
            }
            interface C
            {
                char[] name();
            }
            class D : B, C
            {
                override char[] msg() { return "D"; }
                override char[] name() { return "phil"; }
            }

            struct S { int a, b, c, d; }

            Variant[] scoop(...)
            {
                return Variant.fromVararg(_arguments, _argptr);
            }

            auto va_0 = cast(char)  '?';
            auto va_1 = cast(short) 42;
            auto va_2 = cast(int)   1701;
            auto va_3 = cast(long)  9001;
            auto va_4 = cast(float) 3.14;
            auto va_5 = cast(double)2.14;
            auto va_6 = cast(real)  0.1;
            auto va_7 = "abcd"[];
            S    va_8 = { 1, 2, 3, 4 };
            A    va_9 = new A;
            B    va_a = new B;
            C    va_b = new D;
            D    va_c = new D;
            
            auto vs = scoop(va_0, va_1, va_2, va_3,
                            va_4, va_5, va_6, va_7,
                            va_8, va_9, va_a, va_b, va_c);

            assert( vs[0x0].get!(typeof(va_0)) == va_0 );
            assert( vs[0x1].get!(typeof(va_1)) == va_1 );
            assert( vs[0x2].get!(typeof(va_2)) == va_2 );
            assert( vs[0x3].get!(typeof(va_3)) == va_3 );
            assert( vs[0x4].get!(typeof(va_4)) == va_4 );
            assert( vs[0x5].get!(typeof(va_5)) == va_5 );
            assert( vs[0x6].get!(typeof(va_6)) == va_6 );
            assert( vs[0x7].get!(typeof(va_7)) == va_7 );
            assert( vs[0x8].get!(typeof(va_8)) == va_8 );
            assert( vs[0x9].get!(typeof(va_9)) is va_9 );
            assert( vs[0xa].get!(typeof(va_a)) is va_a );
            assert( vs[0xb].get!(typeof(va_b)) is va_b );
            assert( vs[0xc].get!(typeof(va_c)) is va_c );

            assert( vs[0x9].get!(typeof(va_9)).msg == "A" );
            assert( vs[0xa].get!(typeof(va_a)).msg == "B" );
            assert( vs[0xc].get!(typeof(va_c)).msg == "D" );
            
            assert( vs[0xb].get!(typeof(va_b)).name == "phil" );
            assert( vs[0xc].get!(typeof(va_c)).name == "phil" );

            {
                TypeInfo[] types;
                void* args;

                Variant.toVararg(vs, types, args);

                assert( types[0x0] is typeid(typeof(va_0)) );
                assert( types[0x1] is typeid(typeof(va_1)) );
                assert( types[0x2] is typeid(typeof(va_2)) );
                assert( types[0x3] is typeid(typeof(va_3)) );
                assert( types[0x4] is typeid(typeof(va_4)) );
                assert( types[0x5] is typeid(typeof(va_5)) );
                assert( types[0x6] is typeid(typeof(va_6)) );
                assert( types[0x7] is typeid(typeof(va_7)) );
                assert( types[0x8] is typeid(typeof(va_8)) );
                assert( types[0x9] is typeid(typeof(va_9)) );
                assert( types[0xa] is typeid(typeof(va_a)) );
                assert( types[0xb] is typeid(typeof(va_b)) );
                assert( types[0xc] is typeid(typeof(va_c)) );

                auto ptr = args;

                auto vb_0 = va_arg!(typeof(va_0))(ptr);
                auto vb_1 = va_arg!(typeof(va_1))(ptr);
                auto vb_2 = va_arg!(typeof(va_2))(ptr);
                auto vb_3 = va_arg!(typeof(va_3))(ptr);
                auto vb_4 = va_arg!(typeof(va_4))(ptr);
                auto vb_5 = va_arg!(typeof(va_5))(ptr);
                auto vb_6 = va_arg!(typeof(va_6))(ptr);
                auto vb_7 = va_arg!(typeof(va_7))(ptr);
                auto vb_8 = va_arg!(typeof(va_8))(ptr);
                auto vb_9 = va_arg!(typeof(va_9))(ptr);
                auto vb_a = va_arg!(typeof(va_a))(ptr);
                auto vb_b = va_arg!(typeof(va_b))(ptr);
                auto vb_c = va_arg!(typeof(va_c))(ptr);

                assert( vb_0 == va_0 );
                assert( vb_1 == va_1 );
                assert( vb_2 == va_2 );
                assert( vb_3 == va_3 );
                assert( vb_4 == va_4 );
                assert( vb_5 == va_5 );
                assert( vb_6 == va_6 );
                assert( vb_7 == va_7 );
                assert( vb_8 == va_8 );
                assert( vb_9 is va_9 );
                assert( vb_a is va_a );
                assert( vb_b is va_b );
                assert( vb_c is va_c );

                assert( vb_9.msg == "A" );
                assert( vb_a.msg == "B" );
                assert( vb_c.msg == "D" );

                assert( vb_b.name == "phil" );
                assert( vb_c.name == "phil" );
            }
        }
    }
}
