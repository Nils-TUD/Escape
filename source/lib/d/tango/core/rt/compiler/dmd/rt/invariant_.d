/*
 * Placed into the Public Domain
 * written by Walter Bright
 * www.digitalmars.com
 */
module rt.compiler.dmd.rt.invariant_;

private {
    extern(C) int debugf(char*,...);
}
extern (C) void _d_invariant(Object o)
{   ClassInfo c;

    debugf("__d_invariant_(%x)\n", o);

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
