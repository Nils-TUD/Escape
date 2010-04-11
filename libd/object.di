module object;

alias typeof(int.sizeof)                    size_t;
alias typeof(cast(void*)0 - cast(void*)0)   ptrdiff_t;

alias size_t hash_t;

class Object
{
    char[] toUtf8();
    hash_t toHash();
    int    opCmp(Object o);
    int    opEquals(Object o);

    final void notifyRegister(void delegate(Object) dg);
    final void notifyUnRegister(void delegate(Object) dg);
}

struct Interface
{
    ClassInfo   classinfo;
    void*[]     vtbl;
    int         offset;     // offset to Interface 'this' from Object 'this'
}

class ClassInfo : Object
{
    byte[]      init;       // class static initializer
    char[]      name;       // class name
    void*[]     vtbl;       // virtual function pointer table
    Interface[] interfaces;
    ClassInfo   base;
    void*       destructor;
    void(*classInvariant)(Object);
    uint        flags;
    // 1:                   // IUnknown
    // 2:                   // has no possible pointers into GC memory
    // 4:                   // has offTi[] member
    void*       deallocator;
  version (DigitalMars) {
    OffsetTypeInfo[] offTi;
  }
  void function(Object) defaultConstructor;   // default Constructor
  MemberInfo[] function(char[]) xgetMembers;
}

abstract class MemberInfo
{
    char[] name();
}

class MemberInfo_field : MemberInfo
{
    this(char[] name, TypeInfo ti, size_t offset);

    char[] name();
    TypeInfo typeInfo();
    size_t offset();

    char[]   m_name;
    TypeInfo m_typeinfo;
    size_t   m_offset;
}

class MemberInfo_function : MemberInfo
{
    this(char[] name, TypeInfo ti, void* fp, uint flags);

    char[] name();
    TypeInfo typeInfo();
    void* fp();
    uint flags();

    char[]   m_name;
    TypeInfo m_typeinfo;
    void*    m_fp;
    uint     m_flags;
}

struct OffsetTypeInfo
{
    size_t   offset;
    TypeInfo ti;
}

class TypeInfo
{
    hash_t   getHash(void *p);
    int      equals(void *p1, void *p2);
    int      compare(void *p1, void *p2);
    size_t   tsize();
    void     swap(void *p1, void *p2);
  version( DigitalMars )
  {
    TypeInfo next();
    void[]   init();
    uint     flags();
    // 1:                   // has possible pointers into GC memory
    OffsetTypeInfo[] offTi();
  }
}

class TypeInfo_Typedef : TypeInfo
{
    TypeInfo base;
    char[]   name;
  version( DigitalMars )
    void[]   m_init;
}

class TypeInfo_Enum : TypeInfo_Typedef
{
}

class TypeInfo_Pointer : TypeInfo
{
  version( DigitalMars )
    TypeInfo m_next;
  else
    TypeInfo next;
}

class TypeInfo_Array : TypeInfo
{
  version( DigitalMars )
    TypeInfo value;
  else
    TypeInfo next;
}

class TypeInfo_StaticArray : TypeInfo
{
  version( DigitalMars )
    TypeInfo value;
  else
    TypeInfo next;
    size_t   len;
}

class TypeInfo_AssociativeArray : TypeInfo
{
  version( DigitalMars )
    TypeInfo value;
  else
    TypeInfo next;
    TypeInfo key;
}

class TypeInfo_Function : TypeInfo
{
    TypeInfo next;
}

class TypeInfo_Delegate : TypeInfo
{
    TypeInfo next;
}

class TypeInfo_Class : TypeInfo
{
    ClassInfo info;
}

class TypeInfo_Interface : TypeInfo
{
    ClassInfo info;
}

class TypeInfo_Struct : TypeInfo
{
    char[] name;
  version( DigitalMars )
    void[] m_init;
  else
    byte[] init;

    uint function(void*)      xtoHash;
    int function(void*,void*) xopEquals;
    int function(void*,void*) xopCmp;
    char[] function(void*)    xtoString;

  version( DigitalMars )
    uint m_flags;
}

class TypeInfo_Tuple : TypeInfo
{
    TypeInfo[]  elements;
}

class Exception : Object
{
    char[]      msg;
    char[]      file;
    size_t      line;
    Exception   next;

    this(char[] msg, Exception next = null);
    this(char[] msg, char[] file, size_t line, Exception next = null);
    char[] toUtf8();
}
