/**
 *  Convert any D symbol or type to a human-readable string, at compile time.
 *
 *   Given any D symbol (class, template, function, module name, global, static or local variable)
 *   or any D type, convert it to a compile-time string literal,
 *   optionally containing the fully qualified and decorated name.
 *
 * License:   BSD style: $(LICENSE)
 * Authors:   Don Clugston
 * Copyright: Copyright (C) 2005-2006 Don Clugston
 */
module tango.util.meta.NameOf;
private import tango.util.meta.Demangle;

/*
"At first you may think I'm as mad as a hatter,
when I tell you a cat must have THREE DIFFERENT NAMES."
 -- T.S. Elliot, Old Possum's Book of Practical Cats.
*/

private {
    // --------------------------------------------
    // Here's the magic...
    // This template gives the mangled encoding of F when used as an alias template parameter.
    // This includes the fully qualified name of F, including full type information.
    template Mang(alias F)
    {
        // Make a unique type for each identifier; but don't actually
        // use the identifier for anything.
        // This works because any class, struct or enum always needs to be fully qualified.
        enum mange { ignore }
        // If you take the .mangleof an alias parameter, you are only
        // told that it is an alias.
        // So, we put the type as a function parameter.
        alias void function (mange) mangF;
        // We get the .mangleof for this function pointer. We do this
        // from inside this same template, so that we avoid
        // complications with alias parameters from inner functions.
        // Then we unravel the mangled name to get the alias
// If the identifier is "MyIdentifier" and this module is "QualModule",
// the .mangleof value will be:
//  "PF"   -- because it's a pointer to a function
//  "E"     -- because the first parameter is an enum
//    "4Meta8Demangle"  -- the name of this module
//    "45" -- the number of characters in the remainder of the mangled name.
//      "__T"    -- because it's a class inside a template
//      "4Mang" -- the name of the template "Mang"
//        "T" MyIdentifer -- Here's our prize!
//      "Z"  -- marks the end of the template parameters for "Mang"
//  "5mange" -- this is the enum "mange"
//  "Z"  -- the return value of the function is coming
//  "v"  -- the function returns void
// We take off the "PFE" and "5MangeZv", leaving us with the qualified name of this template.
// Then we extract the last part (which is the unqualified template), then strip off the
// "__T4Mang" and the trailing "Z".
    static if ( typeof(mangF).mangleof[3] > '9') {
//    static assert(0, "Compiler Bug");
    const char [] mangledname = typeof(mangF).mangleof;
                } else
        const char [] mangledname = extractDLastPart!((
                typeof(mangF).mangleof["PFE".length..$-"5mangeZv".length]
            ))["__T4Mang".length..$-1];
   }
}

// --------------------------------------------------------------
// Now, some functions which massage the mangled name to give something more useful.

/**
 * Like .mangleof, except that it works for an alias template parameter instead of a type.
 */
template manglednameof(alias A)
{
   const char [] manglednameof = Mang!(A).mangledname;
}

template manglednameof2(alias A)
{
   const char [] manglednameof = compilerWorkaround!(Mang!(A).mangledname);
}

/** Convert any D type to a human-readable string literal
 *
 * example: "int function(double, char[])"
 */
template prettytypeof(A)
{
  const char [] prettytypeof = demangleType!(A.mangleof, MangledNameType.PrettyName);
}

/**
 * The symbol as it was declared, but including full type qualification.
 *
 * example: "int mymodule.myclass.myfunc(uint, class otherclass)"
 */
template prettynameof(alias A)
{
    // For symbols that don't have D linkage, the type information is not included
    // in the mangled name, so we add it in manually.
  static if (isNonD_SymbolTemplateArg!(manglednameof!(A)) && is(typeof(&A)))
    const char [] prettynameof = prettytypeof!(typeof(A)) ~ " " ~ prettyTemplateArg!(manglednameof!(A), MangledNameType.PrettyName);
  else
    const char [] prettynameof = prettyTemplateArg!(manglednameof!(A), MangledNameType.PrettyName);
}

/**
 * Returns the qualified name of the symbol A.
 *
 * This will be a sequence of identifiers, seperated by dots.
 * eg "mymodule.myclass.myfunc"
 * This is the same as prettynameof(), except that it doesn't include any type information.
 */
template qualifiednameof(alias A)
{
  const char [] qualifiednameof = prettyTemplateArg!(manglednameof!(A), MangledNameType.QualifiedName);
}

/**
 * Returns the unqualified name, as a single text string.
 *
 * eg. "myfunc"
 */
template symbolnameof(alias A)
{
    const char [] symbolnameof = prettyTemplateArg!(manglednameof!(A), MangledNameType.SymbolName);
}

//----------------------------------------------
//                Unit Tests
//----------------------------------------------

debug(UnitTest) {

// remove the ".d" from the end
const char [] THISFILE = "tango.util.meta.NameOf";


private {
// Declare some structs, classes, enums, functions, and templates.

template ClassTemplate(A)
{
   class ClassTemplate {}
}

struct OuterClass  {
class SomeClass {}
}

alias double delegate (int, OuterClass) SomeDelegate;

template IntTemplate(int F)
{
  class IntTemplate { }
}

template MyInt(int F)
{
    const int MyIntX = F;
}


enum SomeEnum { ABC = 2 }
SomeEnum SomeInt;


static assert( prettytypeof!(real) == "real");
static assert( prettytypeof!(OuterClass.SomeClass) == "class " ~ THISFILE ~".OuterClass.SomeClass");

// Test that it works with module names (for example, this module)
static assert( qualifiednameof!(tango.util.meta.NameOf) == THISFILE);
static assert( symbolnameof!(tango.util.meta.NameOf) == "NameOf");
static assert( prettynameof!(tango.util.meta.NameOf) == THISFILE);

static assert( prettynameof!(SomeInt) == "enum " ~ THISFILE ~ ".SomeEnum " ~ THISFILE ~ ".SomeInt");
static assert( qualifiednameof!(OuterClass) == THISFILE ~".OuterClass");
static assert( symbolnameof!(SomeInt) == "SomeInt");
static assert( prettynameof!(ClassTemplate!(OuterClass.SomeClass))
    == "class "~ THISFILE ~ ".ClassTemplate!(class "~ THISFILE ~ ".OuterClass.SomeClass).ClassTemplate");
static assert( symbolnameof!(ClassTemplate!(OuterClass.SomeClass))  == "ClassTemplate");

// Extern(D) declarations have full type information.
extern int pig();
extern int pog;
static assert( prettynameof!(pig) == "int " ~ THISFILE ~ ".pig()");
static assert( prettynameof!(pog) == "int " ~ THISFILE ~ ".pog");
static assert( symbolnameof!(pig) == "pig");

// Extern(Windows) declarations contain no type information.
extern (C) {
    extern int dog(cfloat c) {
        extern(Windows)
        void fog() {
        int fig;

//        static assert( prettynameof!(fig) == "int extern (C) " ~ THISFILE
//        ~ ".dog(cfloat).fog().fig");
    }

        return 0;
    }
    extern int dig;
}

static assert( prettynameof!(dig) == "int dig");
static assert( prettynameof!(dog) == "extern (C) int (cfloat) dog");

// Class inside nested function inside template

extern (Windows) {
template aardvark(X) {
    int aardvark(short goon) {
        class ant {}
        static assert(prettynameof!(ant)== "class " ~ THISFILE ~ ".aardvark!(struct "
            ~ THISFILE ~ ".OuterClass).aardvark(short).ant");
        static assert(qualifiednameof!(ant)== THISFILE ~ ".aardvark.aardvark.ant");
        static assert( symbolnameof!(ant) == "ant");

        return 3;
    }
}

}

// This is just to ensure that the static assert actually gets executed.
const test_aardvark = is (aardvark!(OuterClass) == function);

// Template inside function (only possible with mixins)
template fox(B, ushort C) {
    class fox {}
}

creal wolf(uint a) {

        mixin fox!(cfloat, 21);
        static assert(prettynameof!(fox)== "class " ~ THISFILE ~ ".wolf(uint).fox!(cfloat, int = 21).fox");
        static assert(qualifiednameof!(fox)== THISFILE ~ ".wolf.fox.fox");
        static assert(symbolnameof!(fox)== "fox");
        ushort innerfunc(...)
        {
            wchar innervar;
//            static assert(prettynameof!(innervar)== "wchar " ~ THISFILE ~ ".wolf(uint).innerfunc(...).innervar");
//            static assert(symbolnameof!(innervar)== "innervar");
//            static assert(qualifiednameof!(innervar)== THISFILE ~ ".wolf.innerfunc.innervar");
            return 0;
        }
        return 0+0i;
}

}//private
}//debug
