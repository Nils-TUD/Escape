
// ifloat

module rt.compiler.dmd.typeinfo.ti_ifloat;

private import rt.compiler.dmd.typeinfo.ti_float;

class TypeInfo_o : TypeInfo_f
{
    override char[] toString() { return "ifloat"; }
}
