
module rt.compiler.dmd.typeinfo.ti_Ag;

private import rt.compiler.util.string;
private import rt.compiler.util.hash;
private import tango.stdc.string : memcmp;

// byte[]

class TypeInfo_Ag : TypeInfo_Array
{
    override char[] toString() { return "byte[]"; }

    override hash_t getHash(in void* p) {
        byte[] s = *cast(byte[]*)p;
        size_t len = s.length;
        byte *str = s.ptr;
        return rt_hash_str(str,len*byte.sizeof,0);
    }

    override equals_t equals(in void* p1, in void* p2)
    {
        byte[] s1 = *cast(byte[]*)p1;
        byte[] s2 = *cast(byte[]*)p2;

        return s1.length == s2.length &&
               memcmp(cast(byte *)s1, cast(byte *)s2, s1.length) == 0;
    }

    override int compare(in void* p1, in void* p2)
    {
        byte[] s1 = *cast(byte[]*)p1;
        byte[] s2 = *cast(byte[]*)p2;
        size_t len = s1.length;

        if (s2.length < len)
            len = s2.length;
        for (size_t u = 0; u < len; u++)
        {
            int result = s1[u] - s2[u];
            if (result)
                return result;
        }
        if (s1.length < s2.length)
            return -1;
        else if (s1.length > s2.length)
            return 1;
        return 0;
    }

    override size_t tsize()
    {
        return (byte[]).sizeof;
    }

    override uint flags()
    {
        return 1;
    }

    override TypeInfo next()
    {
        return typeid(byte);
    }
}


// ubyte[]

class TypeInfo_Ah : TypeInfo_Ag
{
    override char[] toString() { return "ubyte[]"; }

    override int compare(in void* p1, in void* p2)
    {
        char[] s1 = *cast(char[]*)p1;
        char[] s2 = *cast(char[]*)p2;

        return stringCompare(s1, s2);
    }

    override TypeInfo next()
    {
        return typeid(ubyte);
    }
}

// void[]

class TypeInfo_Av : TypeInfo_Ah
{
    override char[] toString() { return "void[]"; }

    override TypeInfo next()
    {
        return typeid(void);
    }
}

// bool[]

class TypeInfo_Ab : TypeInfo_Ah
{
    override char[] toString() { return "bool[]"; }

    override TypeInfo next()
    {
        return typeid(bool);
    }
}

// char[]

class TypeInfo_Aa : TypeInfo_Ag
{
    override char[] toString() { return "char[]"; }

    override hash_t getHash(in void* p){
        char[] s = *cast(char[]*)p;
        version (OldHash)
        {
            hash_t hash = 0;
            foreach (char c; s)
                hash = hash * 11 + c;
            return hash;
        } else {
            //return rt_hash_utf8(s,0); // this would be encoding independent
            return rt_hash_str(s.ptr,s.length,0);
        }
    }

    override TypeInfo next()
    {
        return typeid(char);
    }
}
