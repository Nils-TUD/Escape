/**
 * The traits module defines tools useful for obtaining detailed compile-time
 * information about a type.  Please note that the mixed naming scheme used in
 * this module is intentional.  Templates which evaluate to a type follow the
 * naming convention used for types, and templates which evaluate to a value
 * follow the naming convention used for functions.
 *
 * Copyright: Copyright (C) 2005-2006 Sean Kelly.  All rights reserved.
 * License:   BSD style: $(LICENSE)
 * Authors:   Sean Kelly, Fawzi Mohamed
 */
module tango.core.Traits;


/**
 * Evaluates to true if T is char, wchar, or dchar.
 */
template isCharType( T )
{
    const bool isCharType = is( T == char )  ||
                            is( T == wchar ) ||
                            is( T == dchar );
}


/**
 * Evaluates to true if T is a signed integer type.
 */
template isSignedIntegerType( T )
{
    const bool isSignedIntegerType = is( T == byte )  ||
                                     is( T == short ) ||
                                     is( T == int )   ||
                                     is( T == long )/+||
                                     is( T == cent  )+/;
}


/**
 * Evaluates to true if T is an unsigned integer type.
 */
template isUnsignedIntegerType( T )
{
    const bool isUnsignedIntegerType = is( T == ubyte )  ||
                                       is( T == ushort ) ||
                                       is( T == uint )   ||
                                       is( T == ulong )/+||
                                       is( T == ucent  )+/;
}


/**
 * Evaluates to true if T is a signed or unsigned integer type.
 */
template isIntegerType( T )
{
    const bool isIntegerType = isSignedIntegerType!(T) ||
                               isUnsignedIntegerType!(T);
}


/**
 * Evaluates to true if T is a real floating-point type.
 */
template isRealType( T )
{
    const bool isRealType = is( T == float )  ||
                            is( T == double ) ||
                            is( T == real );
}


/**
 * Evaluates to true if T is a complex floating-point type.
 */
template isComplexType( T )
{
    const bool isComplexType = is( T == cfloat )  ||
                               is( T == cdouble ) ||
                               is( T == creal );
}


/**
 * Evaluates to true if T is an imaginary floating-point type.
 */
template isImaginaryType( T )
{
    const bool isImaginaryType = is( T == ifloat )  ||
                                 is( T == idouble ) ||
                                 is( T == ireal );
}


/**
 * Evaluates to true if T is any floating-point type: real, complex, or
 * imaginary.
 */
template isFloatingPointType( T )
{
    const bool isFloatingPointType = isRealType!(T)    ||
                                     isComplexType!(T) ||
                                     isImaginaryType!(T);
}

/// true if T is an atomic type
template isAtomicType(T)
{
    static if( is( T == bool )
            || is( T == char )
            || is( T == wchar )
            || is( T == dchar )
            || is( T == byte )
            || is( T == short )
            || is( T == int )
            || is( T == long )
            || is( T == ubyte )
            || is( T == ushort )
            || is( T == uint )
            || is( T == ulong )
            || is( T == float )
            || is( T == double )
            || is( T == real )
            || is( T == ifloat )
            || is( T == idouble )
            || is( T == ireal ) )
        const isAtomicType = true;
    else
        const isAtomicType = false;
}

/**
 * complex type for the given type
 */
template ComplexTypeOf(T){
    static if(is(T==float)||is(T==ifloat)||is(T==cfloat)){
        alias cfloat ComplexTypeOf;
    } else static if(is(T==double)|| is(T==idouble)|| is(T==cdouble)){
        alias cdouble ComplexTypeOf;
    } else static if(is(T==real)|| is(T==ireal)|| is(T==creal)){
        alias creal ComplexTypeOf;
    } else static assert(0,"unsupported type in ComplexTypeOf "~T.stringof);
}

/**
 * real type for the given type
 */
template RealTypeOf(T){
    static if(is(T==float)|| is(T==ifloat)|| is(T==cfloat)){
        alias float RealTypeOf;
    } else static if(is(T==double)|| is(T==idouble)|| is(T==cdouble)){
        alias double RealTypeOf;
    } else static if(is(T==real)|| is(T==ireal)|| is(T==creal)){
        alias real RealTypeOf;
    } else static assert(0,"unsupported type in RealTypeOf "~T.stringof);
}

/**
 * imaginary type for the given type
 */
template ImaginaryTypeOf(T){
    static if(is(T==float)|| is(T==ifloat)|| is(T==cfloat)){
        alias ifloat ImaginaryTypeOf;
    } else static if(is(T==double)|| is(T==idouble)|| is(T==cdouble)){
        alias idouble ImaginaryTypeOf;
    } else static if(is(T==real)|| is(T==ireal)|| is(T==creal)){
        alias ireal ImaginaryTypeOf;
    } else static assert(0,"unsupported type in ImaginaryTypeOf "~T.stringof);
}

/// type with maximum precision
template MaxPrecTypeOf(T){
    static if (isComplexType!(T)){
        alias creal MaxPrecTypeOf;
    } else static if (isImaginaryType!(T)){
        alias ireal MaxPrecTypeOf;
    } else {
        alias real MaxPrecTypeOf;
    }
}


/**
 * Evaluates to true if T is a pointer type.
 */
template isPointerType(T)
{
        const isPointerType = false;
}

template isPointerType(T : T*)
{
        const isPointerType = true;
}

debug( UnitTest )
{
    unittest
    {
        static assert( isPointerType!(void*) );
        static assert( !isPointerType!(char[]) );
        static assert( isPointerType!(char[]*) );
        static assert( !isPointerType!(char*[]) );
        static assert( isPointerType!(real*) );
        static assert( !isPointerType!(uint) );
        static assert( is(MaxPrecTypeOf!(float)==real));
        static assert( is(MaxPrecTypeOf!(cfloat)==creal));
        static assert( is(MaxPrecTypeOf!(ifloat)==ireal));

        class Ham
        {
            void* a;
        }

        static assert( !isPointerType!(Ham) );

        union Eggs
        {
            void* a;
            uint  b;
        };

        static assert( !isPointerType!(Eggs) );
        static assert( isPointerType!(Eggs*) );

        struct Bacon {};

        static assert( !isPointerType!(Bacon) );

    }
}

/**
 * Evaluates to true if T is a a pointer, class, interface, or delegate.
 */
template isReferenceType( T )
{

    const bool isReferenceType = isPointerType!(T)  ||
                               is( T == class )     ||
                               is( T == interface ) ||
                               is( T == delegate );
}


/**
 * Evaulates to true if T is a dynamic array type.
 */
template isDynamicArrayType( T )
{
    const bool isDynamicArrayType = is( typeof(T.init[0])[] == T );
}

/**
 * Evaluates to true if T is a static array type.
 */
version( GNU )
{
    // GDC should also be able to use the other version, but it probably
    // relies on a frontend fix in one of the latest DMD versions - will
    // remove this when GDC is ready. For now, this code pass the unittests.
    private template isStaticArrayTypeInst( T )
    {
        const T isStaticArrayTypeInst = void;
    }

    template isStaticArrayType( T )
    {
        static if( is( typeof(T.length) ) && !is( typeof(T) == typeof(T.init) ) )
        {
            const bool isStaticArrayType = is( T == typeof(T[0])[isStaticArrayTypeInst!(T).length] );
        }
        else
        {
            const bool isStaticArrayType = false;
        }
    }
}
else
{
    template isStaticArrayType( T : T[U], size_t U )
    {
        const bool isStaticArrayType = true;
    }

    template isStaticArrayType( T )
    {
        const bool isStaticArrayType = false;
    }
}

/// true for array types
template isArrayType(T)
{
    static if (is( T U : U[] ))
        const bool isArrayType=true;
    else
        const bool isArrayType=false;
}

debug( UnitTest )
{
    unittest
    {
        static assert( isStaticArrayType!(char[5][2]) );
        static assert( !isDynamicArrayType!(char[5][2]) );
        static assert( isArrayType!(char[5][2]) );

        static assert( isStaticArrayType!(char[15]) );
        static assert( !isStaticArrayType!(char[]) );

        static assert( isDynamicArrayType!(char[]) );
        static assert( !isDynamicArrayType!(char[15]) );

        static assert( isArrayType!(char[15]) );
        static assert( isArrayType!(char[]) );
        static assert( !isArrayType!(char) );
    }
}

/**
 * Evaluates to true if T is an associative array type.
 */
template isAssocArrayType( T )
{
    const bool isAssocArrayType = is( typeof(T.init.values[0])[typeof(T.init.keys[0])] == T );
}


/**
 * Evaluates to true if T is a function, function pointer, delegate, or
 * callable object.
 */
template isCallableType( T )
{
    const bool isCallableType = is( T == function )             ||
                                is( typeof(*T) == function )    ||
                                is( T == delegate )             ||
                                is( typeof(T.opCall) == function );
}


/**
 * Evaluates to the return type of Fn.  Fn is required to be a callable type.
 */
template ReturnTypeOf( Fn )
{
    static if( is( Fn Ret == return ) )
        alias Ret ReturnTypeOf;
    else
        static assert( false, "Argument has no return type." );
}

/** 
 * Returns the type that a T would evaluate to in an expression.
 * Expr is not required to be a callable type
 */ 
template ExprTypeOf( Expr )
{
    static if(isCallableType!( Expr ))
        alias ReturnTypeOf!( Expr ) ExprTypeOf;
    else
        alias Expr ExprTypeOf;
}


/**
 * Evaluates to the return type of fn.  fn is required to be callable.
 */
template ReturnTypeOf( alias fn )
{
    static if( is( typeof(fn) Base == typedef ) )
        alias ReturnTypeOf!(Base) ReturnTypeOf;
    else
        alias ReturnTypeOf!(typeof(fn)) ReturnTypeOf;
}


/**
 * Evaluates to a tuple representing the parameters of Fn.  Fn is required to
 * be a callable type.
 */
template ParameterTupleOf( Fn )
{
    static if( is( Fn Params == function ) )
        alias Params ParameterTupleOf;
    else static if( is( Fn Params == delegate ) )
        alias ParameterTupleOf!(Params) ParameterTupleOf;
    else static if( is( Fn Params == Params* ) )
        alias ParameterTupleOf!(Params) ParameterTupleOf;
    else
        static assert( false, "Argument has no parameters." );
}


/**
 * Evaluates to a tuple representing the parameters of fn.  n is required to
 * be callable.
 */
template ParameterTupleOf( alias fn )
{
    static if( is( typeof(fn) Base == typedef ) )
        alias ParameterTupleOf!(Base) ParameterTupleOf;
    else
        alias ParameterTupleOf!(typeof(fn)) ParameterTupleOf;
}


/**
 * Evaluates to a tuple representing the ancestors of T.  T is required to be
 * a class or interface type.
 */
template BaseTypeTupleOf( T )
{
    static if( is( T Base == super ) )
        alias Base BaseTypeTupleOf;
    else
        static assert( false, "Argument is not a class or interface." );
}

/**
 * Strips the []'s off of a type.
 */
template BaseTypeOfArrays(T)
{
    static if( is( T S : S[]) ) {
        alias BaseTypeOfArrays!(S)  BaseTypeOfArrays;
    }
    else {
        alias T BaseTypeOfArrays;
    }
}

/**
 * strips one [] off a type
 */
template ElementTypeOfArray(T:T[])
{
    alias T ElementTypeOfArray;
}

/**
 * Count the []'s on an array type
 */
template rankOfArray(T) {
    static if(is(T S : S[])) {
        const uint rankOfArray = 1 + rankOfArray!(S);
    } else {
        const uint rankOfArray = 0;
    }
}

/// type of the keys of an AA
template KeyTypeOfAA(T){
    alias typeof(T.init.keys[0]) KeyTypeOfAA;
}

/// type of the values of an AA
template ValTypeOfAA(T){
    alias typeof(T.init.values[0]) ValTypeOfAA;
}

/// returns the size of a static array
template staticArraySize(T)
{
    static assert(isStaticArrayType!(T),"staticArraySize needs a static array as type");
    static assert(rankOfArray!(T)==1,"implemented only for 1d arrays...");
    const size_t staticArraySize=(T).sizeof / typeof(T.init).sizeof;
}

/// is T is static array returns a dynamic array, otherwise returns T
template DynamicArrayType(T)
{
    static if( isStaticArrayType!(T) )
        alias typeof(T.dup) DynamicArrayType;
    else
        alias T DynamicArrayType;
}

debug( UnitTest )
{
    static assert( is(BaseTypeOfArrays!(real[][])==real) );
    static assert( is(BaseTypeOfArrays!(real[2][3])==real) );
    static assert( is(ElementTypeOfArray!(real[])==real) );
    static assert( is(ElementTypeOfArray!(real[][])==real[]) );
    static assert( is(ElementTypeOfArray!(real[2][])==real[2]) );
    static assert( is(ElementTypeOfArray!(real[2][2])==real[2]) );
    static assert( rankOfArray!(real[][])==2 );
    static assert( rankOfArray!(real[2][])==2 );
    static assert( is(ValTypeOfAA!(char[int])==char));
    static assert( is(KeyTypeOfAA!(char[int])==int));
    static assert( is(ValTypeOfAA!(char[][int])==char[]));
    static assert( is(KeyTypeOfAA!(char[][int[]])==int[]));
    static assert( isAssocArrayType!(char[][int[]]));
    static assert( !isAssocArrayType!(char[]));
    static assert( is(DynamicArrayType!(char[2])==DynamicArrayType!(char[])));
    static assert( is(DynamicArrayType!(char[2])==char[]));
    static assert( staticArraySize!(char[2])==2);
}

// ------- CTFE -------

/// compile time integer to string
char [] ctfe_i2a(int i){
    char[] digit="0123456789";
    char[] res="";
    if (i==0){
        return "0";
    }
    bool neg=false;
    if (i<0){
        neg=true;
        i=-i;
    }
    while (i>0) {
        res=digit[i%10]~res;
        i/=10;
    }
    if (neg)
        return '-'~res;
    else
        return res;
}
/// ditto
char [] ctfe_i2a(long i){
    char[] digit="0123456789";
    char[] res="";
    if (i==0){
        return "0";
    }
    bool neg=false;
    if (i<0){
        neg=true;
        i=-i;
    }
    while (i>0) {
        res=digit[cast(size_t)(i%10)]~res;
        i/=10;
    }
    if (neg)
        return '-'~res;
    else
        return res;
}
/// ditto
char [] ctfe_i2a(uint i){
    char[] digit="0123456789";
    char[] res="";
    if (i==0){
        return "0";
    }
    bool neg=false;
    while (i>0) {
        res=digit[i%10]~res;
        i/=10;
    }
    return res;
}
/// ditto
char [] ctfe_i2a(ulong i){
    char[] digit="0123456789";
    char[] res="";
    if (i==0){
        return "0";
    }
    bool neg=false;
    while (i>0) {
        res=digit[cast(size_t)(i%10)]~res;
        i/=10;
    }
    return res;
}

debug( UnitTest )
{
    unittest {
    static assert( ctfe_i2a(31)=="31" );
    static assert( ctfe_i2a(-31)=="-31" );
    static assert( ctfe_i2a(14u)=="14" );
    static assert( ctfe_i2a(14L)=="14" );
    static assert( ctfe_i2a(14UL)=="14" );
    }
}
