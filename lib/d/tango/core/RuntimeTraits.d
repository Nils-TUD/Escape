/** 
 * Provides runtime traits, which provide much of the functionality of tango.core.Traits and
 * is-expressions, as well as some functionality that is only available at runtime, using 
 * runtime type information. 
 * 
 * Authors: Chris Wright (dhasenan) <dhasenan@gmail.com>
 * License: tango license, apache 2.0
 * Copyright (c) 2009, CHRISTOPHER WRIGHT
 */
module tango.core.RuntimeTraits;

/// If the given type represents a typedef, return the actual type.
TypeInfo realType (TypeInfo type)
{
    // TypeInfo_Typedef.next() doesn't return the actual type.
    // I think it returns TypeInfo_Typedef.base.next().
    // So, a slightly different method.
    auto def = cast(TypeInfo_Typedef) type;
    if (def !is null)
    {
        return def.base;
    }
    return type;
}

/// If the given type represents a class, return its ClassInfo; else return null;
ClassInfo asClass (TypeInfo type)
{
    if (isInterface (type))
    {
        auto klass = cast(TypeInfo_Interface) type;
        return klass.info;
    }
    if (isClass (type))
    {
        auto klass = cast(TypeInfo_Class) type;
        return klass.info;
    }
    return null;
}

/** Returns true iff one type is an ancestor of the other, or if the types are the same.
 * If either is null, returns false. */
bool isDerived (ClassInfo derived, ClassInfo base)
{
    if (derived is null || base is null)
        return false;
    do
        if (derived is base)
            return true;
    while ((derived = derived.base) !is null)
    return false;
}

/** Returns true iff implementor implements the interface described
 * by iface. This is an expensive operation (linear in the number of
 * interfaces and base classes).
 */
bool implements (ClassInfo implementor, ClassInfo iface)
{
    foreach (info; applyInterfaces (implementor))
    {
        if (iface is info)
            return true;
    }
    return false;
}

/** Returns true iff an instance of class test is implicitly castable to target. 
 * This is an expensive operation (isDerived + implements). */
bool isImplicitly (ClassInfo test, ClassInfo target)
{
    // Keep isDerived first.
    // isDerived will be much faster than implements.
    return (isDerived (test, target) || implements (test, target));
}

/** Returns true iff an instance of type test is implicitly castable to target. 
 * If the types describe classes or interfaces, this is an expensive operation. */
bool isImplicitly (TypeInfo test, TypeInfo target)
{
    // A lot of special cases. This is ugly.
    if (test is target)
        return true;
    if (isStaticArray (test) && isDynamicArray (target) && valueType (test) is valueType (target))
    {
        // you can implicitly cast static to dynamic (currently) if they 
        // have the same value type. Other casts should be forbidden.
        return true;
    }
    auto klass1 = asClass (test);
    auto klass2 = asClass (target);
    if (isClass (test) && isClass (target))
    {
        return isDerived (klass1, klass2);
    }
    if (isInterface (test) && isInterface (target))
    {
        return isDerived (klass1, klass2);
    }
    if (klass1 && klass2)
    {
        return isImplicitly (klass1, klass2);
    }
    if (klass1 || klass2)
    {
        // no casts from class to non-class
        return false;
    }
    if ((isSignedInteger (test) && isSignedInteger (target)) || (isUnsignedInteger (test) && isUnsignedInteger (target)) || (isFloat (
            test) && isFloat (target)) || (isCharacter (test) && isCharacter (target)))
    {
        return test.tsize () <= target.tsize ();
    }
    if (isSignedInteger (test) && isUnsignedInteger (target))
    {
        // potential loss of data
        return false;
    }
    if (isUnsignedInteger (test) && isSignedInteger (target))
    {
        // if the sizes are the same, you could be losing data
        // the upper half of the range wraps around to negatives
        // if the target type is larger, you can safely hold it
        return test.tsize () < target.tsize ();
    }
    // delegates and functions: no can do
    // pointers: no
    // structs: no
    return false;
}

///
ClassInfo[] baseClasses (ClassInfo type)
{
    if (type is null)
        return null;
    ClassInfo[] types;
    while ((type = type.base) !is null)
        types ~= type;
    return types;
}

/** Returns a list of all interfaces that this type implements, directly
 * or indirectly. This includes base interfaces of types the class implements,
 * and interfaces that base classes implement, and base interfaces of interfaces
 * that base classes implement. This is an expensive operation. */
ClassInfo[] baseInterfaces (ClassInfo type)
{
    if (type is null)
        return null;
    ClassInfo[] types = directInterfaces (type);
    while ((type = type.base) !is null)
    {
        types ~= interfaceGraph (type);
    }
    return types;
}

/** Returns all the interfaces that this type directly implements, including
 * inherited interfaces. This is an expensive operation.
 * 
 * Examples:
 * ---
 * interface I1 {}
 * interface I2 : I1 {}
 * class A : I2 {}
 * 
 * auto interfaces = interfaceGraph (A.classinfo);
 * // interfaces = [I1.classinfo, I2.classinfo]
 * --- 
 * 
 * ---
 * interface I1 {}
 * interface I2 {}
 * class A : I1 {}
 * class B : A, I2 {}
 * 
 * auto interfaces = interfaceGraph (B.classinfo);
 * // interfaces = [I2.classinfo]
 * ---
 */
ClassInfo[] interfaceGraph (ClassInfo type)
{
    ClassInfo[] info;
    foreach (iface; type.interfaces)
    {
        info ~= iface.classinfo;
        info ~= interfaceGraph (iface.classinfo);
    }
    return info;
}

/** Iterate through all interfaces that type implements, directly or indirectly, including base interfaces. */
struct applyInterfaces
{
    ///
    static applyInterfaces opCall (ClassInfo type)
    {
        applyInterfaces apply;
        apply.type = type;
        return apply;
    }

    ///
    int opApply (int delegate (ref ClassInfo) dg)
    {
        int result = 0;
        for (; type; type = type.base)
        {
            foreach (iface; type.interfaces)
            {
                result = dg (iface.classinfo);
                if (result)
                    return result;
                result = applyInterfaces (iface.classinfo).opApply (dg);
                if (result)
                    return result;
            }
        }
        return result;
    }

    ClassInfo type;
}

///
ClassInfo[] baseTypes (ClassInfo type)
{
    if (type is null)
        return null;
    return baseClasses (type) ~ baseInterfaces (type);
}

///
ModuleInfo moduleOf (ClassInfo type)
{
    foreach (modula; ModuleInfo)
        foreach (klass; modula.localClasses)
            if (klass is type)
                return modula;
    return null;
}

/// Returns a list of interfaces that this class directly implements.
ClassInfo[] directInterfaces (ClassInfo type)
{
    ClassInfo[] types;
    foreach (iface; type.interfaces)
        types ~= iface.classinfo;
    return types;
}

/** Returns a list of all types that are derived from the given type. This does not 
 * count interfaces; that is, if type is an interface, you will only get derived 
 * interfaces back. It is an expensive operations. */
ClassInfo[] derivedTypes (ClassInfo type)
{
    ClassInfo[] types;
    foreach (modula; ModuleInfo)
        foreach (klass; modula.localClasses)
            if (isDerived (klass, type) && (klass !is type))
                types ~= klass;
    return types;
}

///
bool isDynamicArray (TypeInfo type)
{
    // This implementation is evil.
    // Array typeinfos are named TypeInfo_A?, and defined individually for each
    // possible type aside from structs. For example, typeinfo for int[] is
    // TypeInfo_Ai; for uint[], TypeInfo_Ak.
    // So any TypeInfo with length 11 and starting with TypeInfo_A is an array
    // type.
    // Also, TypeInfo_Array is an array type.
    type = realType (type);
    return ((type.classinfo.name[9] == 'A') && (type.classinfo.name.length == 11)) || ((cast(TypeInfo_Array) type) !is null);
}

///
bool isStaticArray (TypeInfo type)
{
    type = realType (type);
    return (cast(TypeInfo_StaticArray) type) !is null;
}

/** Returns true iff the given type is a dynamic or static array (false for associative
 * arrays and non-arrays). */
bool isArray (TypeInfo type)
{
    type = realType (type);
    return isDynamicArray (type) || isStaticArray (type);
}

///
bool isAssociativeArray (TypeInfo type)
{
    type = realType (type);
    return (cast(TypeInfo_AssociativeArray) type) !is null;
}

///
bool isCharacter (TypeInfo type)
{
    type = realType (type);
    return (type is typeid(char) || type is typeid(wchar) || type is typeid(dchar));
}

///
bool isString (TypeInfo type)
{
    type = realType (type);
    return isArray (type) && isCharacter (valueType (type));
}

///
bool isUnsignedInteger (TypeInfo type)
{
    type = realType (type);
    return (type is typeid(uint) || type is typeid(ulong) || type is typeid(ushort) || type is typeid(ubyte));
}

///
bool isSignedInteger (TypeInfo type)
{
    type = realType (type);
    return (type is typeid(int) || type is typeid(long) || type is typeid(short) || type is typeid(byte));
}

///
bool isInteger (TypeInfo type)
{
    type = realType (type);
    return isSignedInteger (type) || isUnsignedInteger (type);
}

///
bool isBool (TypeInfo type)
{
    type = realType (type);
    return (type is typeid(bool));
}

///
bool isFloat (TypeInfo type)
{
    type = realType (type);
    return (type is typeid(float) || type is typeid(double) || type is typeid(real));
}

///
bool isPrimitive (TypeInfo type)
{
    type = realType (type);
    return (isArray (type) || isAssociativeArray (type) || isCharacter (type) || isFloat (type) || isInteger (type));
}

/// Returns true iff the given type represents an interface.
bool isInterface (TypeInfo type)
{
    return (cast(TypeInfo_Interface) type) !is null;
}

///
bool isPointer (TypeInfo type)
{
    type = realType (type);
    return (cast(TypeInfo_Pointer) type) !is null;
}

/// Returns true iff the type represents a class (false for interfaces).
bool isClass (TypeInfo type)
{
    type = realType (type);
    return (cast(TypeInfo_Class) type) !is null;
}

///
bool isStruct (TypeInfo type)
{
    type = realType (type);
    return (cast(TypeInfo_Struct) type) !is null;
}

///
bool isFunction (TypeInfo type)
{
    type = realType (type);
    return ((cast(TypeInfo_Function) type) !is null) || ((cast(TypeInfo_Delegate) type) !is null);
}

/** Returns true iff the given type is a reference type. */
bool isReferenceType (TypeInfo type)
{
    return isClass (type) || isPointer (type) || isDynamicArray (type);
}

/** Returns true iff the given type represents a user-defined type. 
 * This does not include functions, delegates, aliases, or typedefs. */
bool isUserDefined (TypeInfo type)
{
    return isClass (type) || isStruct (type);
}

/** Returns true for all value types, false for all reference types.
 * For functions and delegates, returns false (is this the way it should be?). */
bool isValueType (TypeInfo type)
{
    return !(isDynamicArray (type) || isAssociativeArray (type) || isPointer (type) || isClass (type) || isFunction (
            type));
}

/** The key type of the given type. For an array, size_t; for an associative
 * array T[U], U. */
TypeInfo keyType (TypeInfo type)
{
    type = realType (type);
    auto assocArray = cast(TypeInfo_AssociativeArray) type;
    if (assocArray)
        return assocArray.key;
    if (isArray (type))
        return typeid(size_t);
    return null;
}

/** The value type of the given type -- given T[] or T[n], T; given T[U],
 * T; given T*, T; anything else, null. */
TypeInfo valueType (TypeInfo type)
{
    type = realType (type);
    if (isArray (type))
        return type.next;
    auto assocArray = cast(TypeInfo_AssociativeArray) type;
    if (assocArray)
        return assocArray.value;
    auto pointer = cast(TypeInfo_Pointer) type;
    if (pointer)
        return pointer.m_next;
    return null;
}

/** If the given type represents a delegate or function, the return type
 * of that function. Otherwise, null. */
TypeInfo returnType (TypeInfo type)
{
    type = realType (type);
    auto delegat = cast(TypeInfo_Delegate) type;
    if (delegat)
        return delegat.next;
    auto func = cast(TypeInfo_Function) type;
    if (func)
        return func.next;
    return null;
}

debug (UnitTest)
{

    interface I1
    {
    }

    interface I2
    {
    }

    interface I3
    {
    }

    interface I4
    {
    }

    class A
    {
    }

    class B : A, I1
    {
    }

    class C : B, I2, I3
    {
    }

    class D : A, I1
    {
        int foo (int i)
        {
            return i;
        }
    }

    struct S1
    {
    }

    unittest {
        // Struct-related stuff.
        auto type = typeid(S1);
        assert (isStruct (type));
        assert (isValueType (type));
        assert (isUserDefined (type));
        assert (!isClass (type));
        assert (!isPointer (type));
        assert (null is returnType (type));
        assert (!isPrimitive (type));
        assert (valueType (type) is null);
    }

    unittest {
        auto type = A.classinfo;
        assert (baseTypes (type) == [Object.classinfo]);
        assert (baseClasses (type) == [Object.classinfo]);
        assert (baseInterfaces (type).length == 0);
        type = C.classinfo;
        assert (baseClasses (type) == [B.classinfo, A.classinfo, Object.classinfo]);
        assert (baseInterfaces (type) == [I2.classinfo, I3.classinfo, I1.classinfo]);
        assert (baseTypes (type) == [B.classinfo, A.classinfo, Object.classinfo, I2.classinfo, I3.classinfo,
                I1.classinfo]);
    }

    unittest {
        assert (isPointer (typeid(S1*)));
        assert (isArray (typeid(S1[])));
        assert (valueType (typeid(S1*)) is typeid(S1));
        auto d = new D;
        assert (returnType (typeid(typeof(&d.foo))) is typeid(int));
        assert (isFloat (typeid(real)));
        assert (isFloat (typeid(double)));
        assert (isFloat (typeid(float)));
        assert (!isFloat (typeid(creal)));
        assert (!isFloat (typeid(cdouble)));
        assert (!isInteger (typeid(float)));
        assert (!isInteger (typeid(creal)));
        assert (isInteger (typeid(ulong)));
        assert (isInteger (typeid(ubyte)));
        assert (isCharacter (typeid(char)));
        assert (isCharacter (typeid(wchar)));
        assert (isCharacter (typeid(dchar)));
        assert (!isCharacter (typeid(ubyte)));
        assert (isArray (typeid(typeof("hello"))));
        assert (isCharacter (typeid(typeof("hello"[0]))));
        assert (valueType (typeid(typeof("hello"))) is typeid(typeof('h')));
        assert (isString (typeid(typeof("hello"))), typeof("hello").stringof);
        auto staticString = typeid(typeof("hello"d));
        auto dynamicString = typeid(typeof("hello"d[0 .. $]));
        assert (isString (staticString));
        assert (isStaticArray (staticString));
        assert (isDynamicArray (dynamicString), dynamicString.toString () ~ dynamicString.classinfo.name);
        assert (isString (dynamicString));

        auto type = typeid(int[char[]]);
        assert (valueType (type) is typeid(int), valueType (type).toString ());
        assert (keyType (type) is typeid(char[]), keyType (type).toString ());
        void delegate (int) dg = (int i)
        {
        };
        assert (returnType (typeid(typeof(dg))) is typeid(void));
        assert (returnType (typeid(int delegate (int))) is typeid(int));

        assert (!isDynamicArray (typeid(int[4])));
        assert (isStaticArray (typeid(int[4])));
    }

    unittest {
        typedef int myint;
        assert (typeid(myint) !is null, "null typeid(myint)");
        assert (isInteger (typeid(myint)));
    }

}
