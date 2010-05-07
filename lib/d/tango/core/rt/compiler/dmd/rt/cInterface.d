/+
 + The C interface exported by the runtime, this is the only interface that should be used
 + from outside the runtime
 +
 + Fawzi Mohamed
 +/
module rt.compiler.dmd.rt.cInterface;

// rt.lifetime
struct Array
{
    size_t length;
    byte*  data;
}
struct Array2
{
    size_t length;
    void*  ptr;
}
extern (C) Object _d_newclass(ClassInfo ci);
extern (C) void _d_delinterface(void** p);
extern (C) ulong _d_newarrayT(TypeInfo ti, size_t length);
extern (C) ulong _d_newarrayiT(TypeInfo ti, size_t length);
extern (C) ulong _d_newarraymT(TypeInfo ti, int ndims, ...);
extern (C) ulong _d_newarraymiT(TypeInfo ti, int ndims, ...);
extern (C) void _d_delarray(Array *p);
extern (C) void _d_delmemory(void* *p);
extern (C) void _d_callinterfacefinalizer(void *p);
extern (C) void _d_callfinalizer(void* p);
alias bool function(Object) CollectHandler;
extern (C) void  rt_setCollectHandler(CollectHandler h);
extern (C) void rt_finalize(void* p, bool det = true);
extern (C) byte[] _d_arraysetlengthT(TypeInfo ti, size_t newlength, Array *p);
extern (C) byte[] _d_arraysetlengthiT(TypeInfo ti, size_t newlength, Array *p);
extern (C) long _d_arrayappendT(TypeInfo ti, Array *px, byte[] y);
extern (C) byte[] _d_arrayappendcT(TypeInfo ti, ref byte[] x, ...);
extern (C) byte[] _d_arraycatT(TypeInfo ti, byte[] x, byte[] y);
extern (C) byte[] _d_arraycatnT(TypeInfo ti, uint n, ...);
extern (C) void* _d_arrayliteralT(TypeInfo ti, size_t length, ...);
extern (C) long _adDupT(TypeInfo ti, Array2 a);

// rt.aaA;
alias long ArrayRet_t;
struct aaA
{
    aaA *left;
    aaA *right;
    hash_t hash;
    /* key   */
    /* value */
}
struct BB
{
    aaA*[] b;
    size_t nodes;       // total number of aaA nodes
    TypeInfo keyti;     // TODO: replace this with TypeInfo_AssociativeArray when available in _aaGet()
}
struct AA
{
    BB* a;
}
extern (D) typedef int delegate(void *) dg_t;
extern (D) typedef int delegate(void *, void *) dg2_t;
extern (C) size_t _aaLen(AA aa);
extern (C) void* _aaGet(AA* aa, TypeInfo keyti, size_t valuesize, ...);
extern (C) void* _aaGetRvalue(AA aa, TypeInfo keyti, size_t valuesize, ...);
extern (C) void* _aaIn(AA aa, TypeInfo keyti, ...);
extern (C) void _aaDel(AA aa, TypeInfo keyti, ...);
extern (C) ArrayRet_t _aaValues(AA aa, size_t keysize, size_t valuesize);
extern (C) void* _aaRehash(AA* paa, TypeInfo keyti);
extern (C) void _aaBalance(AA* paa);
extern (C) ArrayRet_t _aaKeys(AA aa, size_t keysize);
extern (C) int _aaApply(AA aa, size_t keysize, dg_t dg);
extern (C) int _aaApply2(AA aa, size_t keysize, dg2_t dg);
extern (C) BB* _d_assocarrayliteralT(TypeInfo_AssociativeArray ti, size_t length, ...);

