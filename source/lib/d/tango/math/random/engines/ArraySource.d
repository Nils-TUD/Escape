/*******************************************************************************
        copyright:      Copyright (c) 2008. Fawzi Mohamed
        license:        BSD style: $(LICENSE)
        version:        Initial release: July 2008
        author:         Fawzi Mohamed
*******************************************************************************/
module tango.math.random.engines.ArraySource;

/// very simple array based source (use with care, some methods in non uniform distributions
/// expect a random source with correct statistics, and could loop forever with such a source)
struct ArraySource{
    uint[] a;
    size_t i;
    const int canCheckpoint=false; // implement?
    const int canSeed=false;
    
    static ArraySource opCall(uint[] a,size_t i=0)
    in { assert(a.length>0,"array needs at least one element"); }
    body {
        ArraySource res;
        res.a=a;
        res.i=i;
        return res;
    }
    uint next(){
        assert(a.length>i,"error, array out of bounds");
        uint el=a[i];
        i=(i+1)%a.length;
        return el;
    }
    ubyte nextB(){
        return cast(ubyte)(0xFF&next);
    }
    ulong nextL(){
        return ((cast(ulong)next)<<32)+cast(ulong)next;
    }
}
