
// ulong

module rt.compiler.dmd.typeinfo.ti_ulong;
private import rt.compiler.util.hash;

class TypeInfo_m : TypeInfo
{
    override char[] toString() { return "ulong"; }

    override hash_t getHash(in void* p)
    {
        static if(hash_t.sizeof==8){
            return cast(hash_t)(*cast(ulong *)p);
        } else {
            return rt_hash_combine(*cast(uint *)p,(cast(uint *)p)[1]);
        }
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        return *cast(ulong *)p1 == *cast(ulong *)p2;
    }

    override int compare(in void* p1, in void* p2)
    {
        if (*cast(ulong *)p1 < *cast(ulong *)p2)
            return -1;
        else if (*cast(ulong *)p1 > *cast(ulong *)p2)
            return 1;
        return 0;
    }

    override size_t tsize()
    {
        return ulong.sizeof;
    }

    override void swap(void *p1, void *p2)
    {
        ulong t;

        t = *cast(ulong *)p1;
        *cast(ulong *)p1 = *cast(ulong *)p2;
        *cast(ulong *)p2 = t;
    }
}
