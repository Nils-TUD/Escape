
// short

module rt.compiler.dmd.typeinfo.ti_short;

class TypeInfo_s : TypeInfo
{
    override char[] toString() { return "short"; }

    override hash_t getHash(in void* p)
    {
        return cast(hash_t)(*cast(ushort *)p);
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        return *cast(short *)p1 == *cast(short *)p2;
    }

    override int compare(in void* p1, in void* p2)
    {
        return *cast(short *)p1 - *cast(short *)p2;
    }

    override size_t tsize()
    {
        return short.sizeof;
    }

    override void swap(void *p1, void *p2)
    {
        short t;

        t = *cast(short *)p1;
        *cast(short *)p1 = *cast(short *)p2;
        *cast(short *)p2 = t;
    }
}
