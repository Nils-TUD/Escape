/**
 * The tuple module defines a template struct used for arbitrary data grouping.
 *
 * Copyright: Copyright (C) 2005-2006 Sean Kelly.  All rights reserved.
 * License:   BSD style: $(LICENSE)
 * Authors:   Walter Bright, Sean Kelly
 */
module tango.core.Tuple;


/**
 * A Tuple is a an aggregate of typed values.  Tuples are useful for returning
 * a set of values from a function or for passing a set of parameters to a
 * function.
 *
 * Example:
 * ----------------------------------------------------------------------
 *
 * alias Tuple!(int, real) T1;
 * alias Tuple!(int, long) T2;
 *
 * T2 func( T1 val )
 * {
 *     T2 ret;
 *     ret[0] = val[0];
 *     ret[1] = val[0] * cast(long) val[1];
 *     return ret;
 * }
 *
 *
 * // tuples may be composed
 * alias Tuple!(int) IntTuple;
 * alias Tuple!(IntTuple, long) RetTuple;
 *
 * // tuples are equivalent to a set of function parameters of the same type
 * RetTuple t = func( 1, 2.3 );
 *
 * ----------------------------------------------------------------------
 */
template Tuple( TList... )
{
    alias TList Tuple;
}


/**
 * Returns the index of the first occurrence of T in TList or Tlist.length if
 * not found.
 */
template IndexOf( T, TList... )
{
    static if( TList.length == 0 )
        const size_t IndexOf = 1;
    else static if( is( T == TList[0] ) )
        const size_t IndexOf = 0;
    else
        const size_t IndexOf = 1 + IndexOf!( T, TList[1 .. $] );
}


/**
 * Returns a Tuple with the first occurrence of T removed from TList.
 */
template Remove( T, TList... )
{
    static if( TList.length == 0 )
        alias TList Remove;
    else static if( is( T == TList[0] ) )
        alias TList[1 .. $] Remove;
    else
        alias Tuple!( TList[0], Remove!( T, TList[1 .. $] ) ) Remove;
}


/**
 * Returns a Tuple with all occurrences of T removed from TList.
 */
template RemoveAll( T, TList... )
{
    static if( TList.length == 0 )
        alias TList RemoveAll;
    else static if( is( T == TList[0] ) )
        alias RemoveAll!( T, TList[1 .. $] ) RemoveAll;
    else
        alias Tuple!( TList[0], RemoveAll!( T, TList[1 .. $] ) ) RemoveAll;
}


/**
 * Returns a Tuple with the first offuccrence of T replaced with U.
 */
template Replace( T, U, TList... )
{
    static if( TList.length == 0 )
        alias TList Replace;
    else static if( is( T == TList[0] ) )
        alias Tuple!(U, TList[1 .. $]) Replace;
    else
        alias Tuple!( TList[0], Replace!( T, U, TList[1 .. $] ) ) Replace;
}


/**
 * Returns a Tuple with all occurrences of T replaced with U.
 */
template ReplaceAll( T, U, TList... )
{
    static if( TList.length == 0 )
        alias TList ReplaceAll;
    else static if( is( T == TList[0] ) )
        alias Tuple!( U, ReplaceAll!( T, U, TList[1 .. $] ) ) ReplaceAll;
    else
        alias Tuple!( TList[0], ReplaceAll!( T, U, TList[1 .. $] ) ) ReplaceAll;
}


/**
 * Returns a Tuple with the types from TList declared in reverse order.
 */
template Reverse( TList... )
{
    static if( TList.length == 0 )
        alias TList Reverse;
    else
        alias Tuple!( Reverse!( TList[1 .. $]), TList[0] ) Reverse;
}


/**
 * Returns a Tuple with all duplicate types removed.
 */
template Unique( TList... )
{
    static if( TList.length == 0 )
        alias TList Unique;
    else
        alias Tuple!( TList[0],
                      Unique!( RemoveAll!( TList[0],
                                           TList[1 .. $] ) ) ) Unique;
}


/**
 * Returns the type from TList that is the most derived from T.  If no such
 * type is found then T will be returned.
 */
template MostDerived( T, TList... )
{
    static if( TList.length == 0 )
        alias T MostDerived;
    else static if( is( TList[0] : T ) )
        alias MostDerived!( TList[0], TList[1 .. $] ) MostDerived;
    else
        alias MostDerived!( T, TList[1 .. $] ) MostDerived;
}


/**
 * Returns a Tuple with the types sorted so that the most derived types are
 * ordered before the remaining types.
 */
template DerivedToFront( TList... )
{
    static if( TList.length == 0 )
        alias TList DerivedToFront;
    else
        alias Tuple!( MostDerived!( TList[0], TList[1 .. $] ),
	                  DerivedToFront!( ReplaceAll!( MostDerived!( TList[0], TList[1 .. $] ),
						                            TList[0],
						                            TList[1 .. $] ) ) ) DerivedToFront;
}
