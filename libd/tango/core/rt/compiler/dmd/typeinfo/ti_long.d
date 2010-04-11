
// long

module rt.compiler.dmd.typeinfo.ti_long;
private import rt.compiler.util.hash;

class TypeInfo_l : TypeInfo
{
    override char[] toString() { return "long"; }

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
        return *cast(long *)p1 == *cast(long *)p2;
    }

    override int compare(in void* p1, in void* p2)
    {
        if (*cast(long *)p1 < *cast(long *)p2)
            return -1;
        else if (*cast(long *)p1 > *cast(long *)p2)
            return 1;
        return 0;
    }

    override size_t tsize()
    {
        return long.sizeof;
    }

    override void swap(void *p1, void *p2)
    {
        long t;

        t = *cast(long *)p1;
        *cast(long *)p1 = *cast(long *)p2;
        *cast(long *)p2 = t;
    }
}
