
// cdouble

module rt.compiler.dmd.typeinfo.ti_cdouble;
private import rt.compiler.util.hash;

class TypeInfo_r : TypeInfo
{
    override char[] toString() { return "cdouble"; }

    override hash_t getHash(in void* p)
    {
        return rt_hash_str(p,cdouble.sizeof,0);
    }

    static equals_t _equals(cdouble f1, cdouble f2)
    {
        return f1 == f2;
    }

    static int _compare(cdouble f1, cdouble f2)
    {   int result;

        if (f1.re < f2.re)
            result = -1;
        else if (f1.re > f2.re)
            result = 1;
        else if (f1.im < f2.im)
            result = -1;
        else if (f1.im > f2.im)
            result = 1;
        else
            result = 0;
        return result;
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        return _equals(*cast(cdouble *)p1, *cast(cdouble *)p2);
    }

    override int compare(in void* p1, in void* p2)
    {
        return _compare(*cast(cdouble *)p1, *cast(cdouble *)p2);
    }

    override size_t tsize()
    {
        return cdouble.sizeof;
    }

    override void swap(void *p1, void *p2)
    {
        cdouble t;

        t = *cast(cdouble *)p1;
        *cast(cdouble *)p1 = *cast(cdouble *)p2;
        *cast(cdouble *)p2 = t;
    }

    override void[] init()
    {   static cdouble r;

        return (cast(cdouble *)&r)[0 .. 1];
    }
}
