/*
 * Placed into the Public Domain
 * written by Walter Bright
 * www.digitalmars.com
 */

private {
    extern(C) int debugf(char*,...);
    extern(C) void printStackTrace();
}
void _d_invariant(Object o)
{   ClassInfo c;

    debugf("__d_invariant(%x)\n", o);
    if(cast(void*)o == cast(void*)0x98e00)
    	printStackTrace();

    // BUG: needs to be filename/line of caller, not library routine
    assert(o !is null); // just do null check, not invariant check
    c = o.classinfo;
    do
    {
        if (c.classInvariant !is null)
        {
            void delegate() inv;
            inv.ptr = cast(void*) o;
            inv.funcptr =  cast(void function()) c.classInvariant;
            inv();
        }
        c = c.base;
    } while (c);
}
