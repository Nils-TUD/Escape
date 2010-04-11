/**
 *  Demangle a ".mangleof" name at compile time.
 *
 * Used by tango.meta.Nameof.
 *
 * License:   BSD style: $(LICENSE)
 * Authors:   Don Clugston
 * Copyright: Copyright (C) 2005-2006 Don Clugston
 */
module tango.util.meta.Demangle;
private import tango.util.meta.Convert;
/*
 Implementation is via pairs of metafunctions:
 a 'demangle' metafunction, which returns a const char [],
 and a 'Consumed' metafunction, which returns an integer, the number of characters which
 are used.
*/



/*****************************************
 * How should the name be displayed?
 */
enum MangledNameType
{
    PrettyName,    // With full type information
    QualifiedName, // No type information, just identifiers seperated by dots
    SymbolName     // Only the ultimate identifier
}

/*****************************************
 * Pretty-prints a mangled type string.
 */
template demangleType(char[] str, MangledNameType wantQualifiedNames = MangledNameType.PrettyName)
{
    static if (wantQualifiedNames != MangledNameType.PrettyName) {
        // symbolnameof!(), qualifiednameof!() only make sense for
        // user-defined types.
        static if (str[0]=='C' || str[0]=='S' || str[0]=='E' || str[0]=='T')
            const char [] demangleType = prettyLname!(str[1..$], wantQualifiedNames);
        else {
            static assert(0, "Demangle error: type '" ~ str ~ "' does not contain a qualified name");
        }
    } else static if (str[0] == 'A') // dynamic array
        const char [] demangleType = demangleType!(str[1..$], wantQualifiedNames) ~ "[]";
    else static if (str[0] == 'H')   // associative array
        const char [] demangleType
            = demangleType!(str[1+demangleTypeConsumed!(str[1..$])..$], wantQualifiedNames)
            ~ "[" ~ demangleType!(str[1..1+(demangleTypeConsumed!(str[1..$]))], wantQualifiedNames) ~ "]";
    else static if (str[0] == 'G') // static array
        const char [] demangleType
            = demangleType!(str[1+countLeadingDigits!(str[1..$])..$], wantQualifiedNames)
            ~ "[" ~ str[1..1+countLeadingDigits!(str[1..$]) ] ~ "]";
    else static if (str[0]=='C')
        const char [] demangleType = "class " ~ prettyLname!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='S')
        const char [] demangleType = "struct " ~ prettyLname!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='E')
        const char [] demangleType = "enum " ~ prettyLname!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='T')
        const char [] demangleType = "typedef " ~ prettyLname!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='D' && str.length>2 && isMangledFunction!(( str[1] )) ) // delegate
        const char [] demangleType = prettyFunctionOrDelegate!(str[1..$], "delegate ", wantQualifiedNames);
    else static if (str[0]=='P' && str.length>2 && isMangledFunction!(( str[1] )) ) // function pointer
        const char [] demangleType = prettyFunctionOrDelegate!(str[1..$], "function ", wantQualifiedNames);
    else static if (str[0]=='P') // only after we've dealt with function pointers
        const char [] demangleType = demangleType!(str[1..$], wantQualifiedNames) ~ "*";
    else static if (str[0]=='M' && isMangledFunction!((str[1])))
       const char [] demangleType = prettyFunctionOrDelegate!(str[1..$], "", wantQualifiedNames);
    else static if (isMangledFunction!((str[0])))
        const char [] demangleType = prettyFunctionOrDelegate!(str, "", wantQualifiedNames);
    else const char [] demangleType = prettyBasicType!(str);
}

// When str contains a mangled function, function pointer, or delegate,
// return the type of the mangled first parameter.
template extractParameterList(char [] str)
{
    static if (str[0]=='D' && str.length>2 && isMangledFunction!(( str[1] )) ) // delegate
        const char [] extractParameterList = str[2..2+demangleParamListConsumed!(str[2..$])];
    else static if (str[0]=='P' && str.length>2 && isMangledFunction!(( str[1] )) ) // function pointer
        const char [] extractParameterList = str[2..2+demangleParamListConsumed!(str[2..$])];
    else static if (isMangledFunction!((str[0])))
        const char [] extractParameterList = str[1..1+demangleParamListConsumed!(str[1..$])];
    else static assert("Not a function or delegate");
}

// How many characters are used in the parameter list and return value?
template demangleParamListConsumed(char [] str)
{
    static assert (str.length>0, "Demangle error(ParamList): No return value found");
    static if (str[0]=='Z' || str[0]=='Y' || str[0]=='X')
        const int demangleParamListConsumed = 0;
    else {
        const int demangleParamListConsumed = demangleFunctionParamTypeConsumed!(str)
            + demangleParamListConsumed!(str[demangleFunctionParamTypeConsumed!(str)..$]);
    }
}

// split these off because they're numerous and simple
template prettyBasicType(char [] str)
{
         static if (str == "v") const char [] prettyBasicType = "void";
    else static if (str == "b") const char [] prettyBasicType = "bool";
    // integral types
    else static if (str == "g") const char [] prettyBasicType = "byte";
    else static if (str == "h") const char [] prettyBasicType = "ubyte";
    else static if (str == "s") const char [] prettyBasicType = "short";
    else static if (str == "t") const char [] prettyBasicType = "ushort";
    else static if (str == "i") const char [] prettyBasicType = "int";
    else static if (str == "k") const char [] prettyBasicType = "uint";
    else static if (str == "l") const char [] prettyBasicType = "long";
    else static if (str == "m") const char [] prettyBasicType = "ulong";
    // floating point
    else static if (str == "e") const char [] prettyBasicType = "real";
    else static if (str == "d") const char [] prettyBasicType = "double";
    else static if (str == "f") const char [] prettyBasicType = "float";

    else static if (str == "j") const char [] prettyBasicType = "ireal";
    else static if (str == "p") const char [] prettyBasicType = "idouble";
    else static if (str == "o") const char [] prettyBasicType = "ifloat";

    else static if (str == "c") const char [] prettyBasicType = "creal";
    else static if (str == "r") const char [] prettyBasicType = "cdouble";
    else static if (str == "q") const char [] prettyBasicType = "cfloat";
    // Char types
    else static if (str == "a") const char [] prettyBasicType = "char";
    else static if (str == "u") const char [] prettyBasicType = "wchar";
    else static if (str == "w") const char [] prettyBasicType = "dchar";

    else static assert(0, "Demangle Error: '" ~ str ~ "' is not a recognised type");
}

template isMangledBasicType(char [] str)
{
    const bool isMangledBasicType =    ((str == "v") || (str == "b")
     || (str == "g") || (str == "h") || (str == "s") || (str == "t")
     || (str == "i") || (str == "k") || (str == "l") || (str == "m")
     || (str == "e") || (str == "d") || (str == "f")
     || (str == "j") || (str == "p") || (str == "o")
     || (str == "c") || (str == "r") || (str == "q")
     || (str == "a") || (str == "u") || (str == "w")
    );
}

template demangleTypeConsumed(char [] str)
{
    static if (str[0]=='A')
        const int demangleTypeConsumed = 1 + demangleTypeConsumed!(str[1..$]);
    else static if (str[0]=='H')
        const int demangleTypeConsumed = 1 + demangleTypeConsumed!(str[1..$])
            + demangleTypeConsumed!(str[1+demangleTypeConsumed!(str[1..$])..$]);
    else static if (str[0]=='G')
        const int demangleTypeConsumed = 1 + countLeadingDigits!(str[1..$])
            + demangleTypeConsumed!( str[1+countLeadingDigits!(str[1..$])..$] );
    else static if (str.length>2 && (str[0]=='P' || str[0]=='D') && isMangledFunction!(( str[1] )) )
        const int demangleTypeConsumed = 2 + demangleParamListAndRetValConsumed!(str[2..$]);
    else static if (str[0]=='P') // only after we've dealt with function pointers
        const int demangleTypeConsumed = 1 + demangleTypeConsumed!(str[1..$]);
    else static if (str[0]=='C' || str[0]=='S' || str[0]=='E' || str[0]=='T')
        const int demangleTypeConsumed = 1 + getDotNameConsumed!(str[1..$]);
    else static if (str[0]=='M' && isMangledFunction!((str[1])))
        const int demangleTypeConsumed = 2 + demangleParamListAndRetValConsumed!(str[2..$]);
    else static if (isMangledFunction!((str[0])) && str.length>1)
        const int demangleTypeConsumed = 1 + demangleParamListAndRetValConsumed!(str[1..$]);
    else static if (isMangledBasicType!(str[0..1])) // it's a Basic Type
        const int demangleTypeConsumed = 1;
    else static assert(0, "Demangle Error: '" ~ str ~ "' is not a recognised basic type");
}

// --------------------------------------------
//              LNAMES

// str must start with an Lname: first chars give the length
// reads the digits from the front of str, gets the Lname
// Sometimes the characters following the length are also digits!
// (this happens with templates, when the name being 'lengthed' is itself an Lname).
// We guard against this by ensuring that the L is less than the length of the string.
template extractLname(char [] str)
{
    static if (str.length <= 9+1 || !beginsWithDigit!(str[1..$]) )
        const char [] extractLname = str[1..(str[0]-'0' + 1)];
    else static if (str.length <= 99+2 || !beginsWithDigit!(str[2..$]) )
        const char [] extractLname = str[2..((str[0]-'0')*10 + str[1]-'0'+ 2)];
    else static if (str.length <= 999+3 || !beginsWithDigit!(str[3..$]) )
        const char [] extractLname =
            str[3..((str[0]-'0')*100 + (str[1]-'0')*10 + str[2]-'0' + 3)];
    else
        const char [] extractLname =
            str[4..((str[0]-'0')*1000 + (str[1]-'0')*100 + (str[2]-'0')*10 + (str[3]-'0') + 4)];
}

// str must start with an lname: first chars give the length
// Returns the number of characters used by length digits + the name itself
template getLnameConsumed(char [] str)
{
    static if (str.length==0)
        const int getLnameConsumed=0;
    else static if (str.length <= (9+1) || !beginsWithDigit!(str[1..$]) )
        const int getLnameConsumed = 1 + str[0]-'0';
    else static if (str.length <= (99+2) || !beginsWithDigit!( str[2..$]) )
        const int getLnameConsumed = (str[0]-'0')*10 + str[1]-'0' + 2;
    else static if (str.length <= (999+3) || !beginsWithDigit!( str[3..$]) )
        const int getLnameConsumed = (str[0]-'0')*100 + (str[1]-'0')*10 + str[2]-'0' + 3;
    else
        const int getLnameConsumed = (str[0]-'0')*1000 + (str[1]-'0')*100 + (str[2]-'0')*10 + (str[3]-'0') + 4;
}

// True if str is a possible continuation of a _D name.
template continues_Dname(char [] str)
{
    static if (str.length>0) {
        const bool continues_Dname = (str[0]=='M') || beginsWithDigit!(str);
    } else static assert(0);
}

// Could be either an Lname, or an Lname followed by a function; but cannot
// be the full length of the string
//
template functionalLnameConsumed(char [] str)
{
    static if ( str.length<1) const int get_DQualifiedNameConsumed = 0;

}

// for an Lname that begins with "_D".
// There must be a type at the end of the name, so the whole string must not be
// consumed.
template get_DQualifiedNameConsumed (char [] str)
{
    static if ( str.length<1) const int get_DQualifiedNameConsumed = 0;
    else static if ( beginsWithDigit!(str) ) {
        const int get_DQualifiedNameConsumed = getLnameConsumed!(str) + get_DQualifiedNameConsumed!(str[getLnameConsumed!(str)..$]);
    } else static if (str[0]=='M' && isMangledFunction!((str[1]))) {
        static if (demangleTypeConsumed!(str)!=str.length && continues_Dname!(str[demangleTypeConsumed!(str)..$])) {
            const int get_DQualifiedNameConsumed = demangleTypeConsumed!(str) + get_DQualifiedNameConsumed!(str[demangleTypeConsumed!(str)..$]);
        } else static assert(0, "Bad qualified name: " ~ str);
    } else static if (isMangledFunction!((str[0]))) {
        static if (demangleTypeConsumed!(str)!=str.length && continues_Dname!(str[demangleTypeConsumed!(str)..$])) {
            const int get_DQualifiedNameConsumed = demangleTypeConsumed!(str) + get_DQualifiedNameConsumed!(str[demangleTypeConsumed!(str)..$]);
        } else const int get_DQualifiedNameConsumed = 0;
    } else const int get_DQualifiedNameConsumed = 0;
}

// don't display return value.
template prettyInner_DFunc(char [] funcname, char [] functype, MangledNameType wantQualifiedNames)
{
    static if (wantQualifiedNames == MangledNameType.PrettyName) {
        const char [] prettyInner_DFunc = prettyExtern!((functype[0])) ~ prettyDotName!(funcname, wantQualifiedNames) ~ "(" ~ prettyParamList!(functype[1..$], MangledNameType.PrettyName)~ ")";
    } else const char [] prettyInner_DFunc = prettyDotName!(funcname, wantQualifiedNames);
}

template qualifiedAndFuncConsumed(char [] str)
{
    const int qualifiedAndFuncConsumed =
            getDotNameConsumed!(str) + 1 + demangleParamListAndRetValConsumed!(str[1+getDotNameConsumed!(str)..$]);
}


// Given a dotname, extract the last component of it.
template extractLastPartOfDotName(char [] str)
{
    static if ( getLnameConsumed!(str) < str.length && beginsWithDigit!(str[getLnameConsumed!(str)..$]) ) {
                const char [] extractLastPartOfDotName =
                    extractLastPartOfDotName!(str[getLnameConsumed!(str) .. $], wantQualifiedNames, "");
    } else const char [] extractLastPartOfDotName = extractLname!(str);
}

// str must be a qualified name
template extractDLastPart(char [] str)
{
    static if (getLnameConsumed!(str)==str.length) {
        const char [] extractDLastPart =  extractLastPartOfDotName!(str);
    } else static if (beginsWithDigit!(str[getLnameConsumed!(str)..$])){
        const char [] extractDLastPart = extractDLastPart!(str[getLnameConsumed!(str)..$]);
    } else static if (continues_Dname!(str[qualifiedAndFuncConsumed!(str)..$])) {
        const char [] extractDLastPart = extractDLastPart!(str[qualifiedAndFuncConsumed!(str)..$]);
    } else const char [] extractDLastPart = extractLname!(str[0..qualifiedAndFuncConsumed!(str)]);
}

// Deal with the case where an Lname contains an embedded "__D".
// This can happen when classes, typedefs, etc are declared inside a function.
// It always starts with a qualified name, but it may be an inner function.
template pretty_Dname(char [] str, MangledNameType wantQualifiedNames)
{
    static if (getDotNameConsumed!(str)==str.length) {
        const char [] pretty_Dname = prettyDotName!(str, wantQualifiedNames);
    } else static if ( str[getDotNameConsumed!(str)]!='M' && !isMangledFunction!( (str[getDotNameConsumed!(str)]))) {
        static assert(0, "Demangle error, not a qualified name or inner function: " ~ str);
    } else {
        // Inner function
        static if (str[getDotNameConsumed!(str)]=='M') {
            static assert(0, "Inner function, not yet implemented: " ~ str);
        }
        else
        static if(continues_Dname!(str[qualifiedAndFuncConsumed!(str)..$])) {
            static if (wantQualifiedNames == MangledNameType.SymbolName) {
                const char [] pretty_Dname =
                    pretty_Dname!(str[qualifiedAndFuncConsumed!(str)..$], wantQualifiedNames);
            } else {
                const char [] pretty_Dname =
                    prettyInner_DFunc!(
                        str[0..getDotNameConsumed!(str)],
                        str[getDotNameConsumed!(str)..qualifiedAndFuncConsumed!(str)], wantQualifiedNames
                    )
                    ~ "." ~ pretty_Dname!(str[qualifiedAndFuncConsumed!(str)..$], wantQualifiedNames);
            }
        } else {
            static assert(0);
        }
    }
}

template showTypeWithName(char [] namestr, char [] typestr, MangledNameType wantQualifiedNames)
{
    static if (wantQualifiedNames == MangledNameType.PrettyName) {
        static if ( isMangledFunction!( (typestr[0])) ) {
            const char [] showTypeWithName = demangleReturnValue!(typestr[1..$], wantQualifiedNames)
                ~ " " ~ pretty_Dname!(namestr, wantQualifiedNames)
                ~ "(" ~ prettyParamList!(typestr[1..$], MangledNameType.PrettyName)~ ")";
        } else {
            const char [] showTypeWithName = demangleType!(typestr) ~ " " ~ pretty_Dname!(namestr, wantQualifiedNames);
        }
    } else const char [] showTypeWithName = pretty_Dname!(namestr, wantQualifiedNames);
}

/* Pretty-print a single component of an Lname.
 * A name fragment is one of:
 *  a DotName
 *  a _D extern name
 *  a __T template
 *  an extern(Windows/Pascal/C/C++) symbol
 */
// Templates and _D qualified names are treated specially.
template prettyLname(char [] str, MangledNameType wantQualifiedNames)
{
    static if (str.length>3 && str[0..3] == "__T") // Template instance name
        static if (wantQualifiedNames == MangledNameType.PrettyName) {
            const char [] prettyLname =
                prettyLname!(str[3..3+getDotNameConsumed!(str[3..$])], wantQualifiedNames) ~ "!("
                ~ prettyTemplateArgList!(str[3+getDotNameConsumed!(str[3..$])..$], wantQualifiedNames)
                ~ ")";
        } else {
            const char [] prettyLname =
                prettyLname!(str[3..3+getDotNameConsumed!(str[3..$])], wantQualifiedNames);
        }
    else static if (str.length>2 && str[0..2] == "_D") {
        static if (2+get_DQualifiedNameConsumed!(str[2..$])== str.length) {
            // it's just a name
            const char [] prettyLname = pretty_Dname!(str[2..2+get_DQualifiedNameConsumed!(str[2..$])], wantQualifiedNames);
        } else { // it has type information following
            const char [] prettyLname = showTypeWithName!(str[2..2+get_DQualifiedNameConsumed!(str[2..$])], str[2+get_DQualifiedNameConsumed!(str[2..$])..$], wantQualifiedNames);
        }
    } else static if ( beginsWithDigit!( str ) ) {
        static if (get_DQualifiedNameConsumed!(str)==str.length) {
            const char [] prettyLname = prettyDotName!(str, wantQualifiedNames);
        } else static assert(0, "Demangle Error: Wrong dot name length for: " ~ str ~ " Consumed=" ~str[0..getDotNameConsumed!(str)]);
    } else {
         // For extern(Pascal/Windows/C) functions.
         // BUG: This case is ambiguous, since type information is lost.
        const char [] prettyLname = str;
    }
}

// a DotName is a sequence of Lnames, seperated by dots.
template prettyDotName(char [] str, MangledNameType wantQualifiedNames, char [] dotstr = "")
{
    static if (str.length==0) const char [] prettyDotName="";
    else {
   //     static assert (beginsWithDigit!(str));
        static if ( getLnameConsumed!(str) < str.length && beginsWithDigit!(str[getLnameConsumed!(str)..$]) ) {
            static if (wantQualifiedNames == MangledNameType.SymbolName) {
                // For symbol names, only display the last symbol
                const char [] prettyDotName =
                    prettyDotName!(str[getLnameConsumed!(str) .. $], wantQualifiedNames, "");
            } else {
                // Qualified and pretty names display everything
                const char [] prettyDotName = dotstr
                    ~ prettyLname!(extractLname!(str), wantQualifiedNames)
                    ~ prettyDotName!(str[getLnameConsumed!(str) .. $], wantQualifiedNames, ".");
            }
        } else static if (getLnameConsumed!(str) < str.length && str[getLnameConsumed!(str)]=='M') {
            static if (wantQualifiedNames == MangledNameType.SymbolName) {
                // For symbol names, only display the last symbol
                const char [] prettyDotName =
                    prettyDotName!(str[getLnameConsumed!(str)+1+demangleTypeConsumed!(str[getLnameConsumed!(str)+1..$]) .. $], wantQualifiedNames, "");
            } else static if (wantQualifiedNames == MangledNameType.QualifiedName) {
                const char [] prettyDotName = dotstr
                    ~ prettyLname!(extractLname!(str), wantQualifiedNames)
                    ~ prettyDotName!(str[getLnameConsumed!(str)+1+demangleTypeConsumed!(str[getLnameConsumed!(str)+1..$]) .. $], wantQualifiedNames, ".");
            } else {
                // Pretty names display everything
                const char [] prettyDotName = dotstr
                    ~ prettyLname!(extractLname!(str), wantQualifiedNames) ~ "("
                    ~ prettyParamList!(str[getLnameConsumed!(str)+2..$], wantQualifiedNames) ~ ")"
                    ~ prettyDotName!(str[getLnameConsumed!(str)+1+demangleTypeConsumed!(str[getLnameConsumed!(str)+1..$]) .. $], wantQualifiedNames, ".");
            }
        } else static if (getLnameConsumed!(str)==str.length) {
            const char [] prettyDotName = dotstr ~ prettyLname!(extractLname!(str), wantQualifiedNames);
        } else {
            static assert(get_DQualifiedNameConsumed!(str)==str.length, "Demangle error: Unexpected "~ str);
            static if (wantQualifiedNames == MangledNameType.SymbolName) {
                // For symbol names, only display the last symbol
                const char [] prettyDotName =
                    prettyDotName!(str[getLnameConsumed!(str)+demangleTypeConsumed!(str[getLnameConsumed!(str)..$]) .. $], wantQualifiedNames, "");
            } else static if (wantQualifiedNames == MangledNameType.QualifiedName) {
                const char [] prettyDotName = dotstr
                    ~ prettyLname!(extractLname!(str), wantQualifiedNames)
                    ~ prettyDotName!(str[getLnameConsumed!(str)+demangleTypeConsumed!(str[getLnameConsumed!(str)..$]) .. $], wantQualifiedNames, ".");
            } else
            const char [] prettyDotName = dotstr ~ prettyLname!(extractLname!(str), wantQualifiedNames)
            ~ "(" ~ prettyParamList!(str[1+getLnameConsumed!(str)..$], wantQualifiedNames) ~ ")"
            ~ dotstr ~ prettyDotName!(str[1+getLnameConsumed!(str) + demangleParamListAndRetValConsumed!(str[1+getLnameConsumed!(str)..$])..$],wantQualifiedNames);
//               ~ prettyDotName!(str[getLnameConsumed!(str)..$], );
//            const char [] prettyDotName = dotstr ~ prettyLname!(str, wantQualifiedNames);
        }
    }
}

// previous letter was an 'M'.
template getDotNameMConsumed(char [] str)
{
    const int getDotNameMConsumed = demangleTypeConsumed!(str)
    + getDotNameConsumed!(str[demangleTypeConsumed!(str)..$]);
}

template getDotNameConsumed (char [] str)
{
    static if ( str.length>1 &&  beginsWithDigit!(str) ) {
        static if (getLnameConsumed!(str) == str.length)
            const int getDotNameConsumed = str.length;
        else static if (beginsWithDigit!( str[getLnameConsumed!(str)..$])) {
            const int getDotNameConsumed = getLnameConsumed!(str)
                    + getDotNameConsumed!(str[getLnameConsumed!(str) .. $]);
        } else static if (str[getLnameConsumed!(str)]=='M') {
            const int getDotNameConsumed = getLnameConsumed!(str) + 1 +
                getDotNameMConsumed!(str[1+getLnameConsumed!(str)..$]);
//        } else static if (isMangledFunction!((str[getLnameConsumed!(str)]))) {
//            const int getDotNameConsumed = getLnameConsumed!(str) +
//            getDotNameMConsumed!(str[getLnameConsumed!(str)..$]);
        }else {
            const int getDotNameConsumed = getLnameConsumed!(str);
        }
    } else static if (str.length>1 && str[0..2] == "_D" ) {
        const int getDotNameConsumed = 2 + get_DQualifiedNameConsumed!(str[2..$]);
    } else static assert(0, "Error in Dot name:" ~ str);
}

// ----------------------------------------
//              FUNCTIONS

/* str[0] must indicate the extern linkage of the function. funcOrDelegStr is the name of the function,
* or "function " or "delegate "
*/
template prettyFunctionOrDelegate(char [] str, char [] funcOrDelegStr, MangledNameType wantQualifiedNames)
{
    const char [] prettyFunctionOrDelegate = prettyExtern!(( str[0] ))
        ~ demangleReturnValue!(str[1..$], wantQualifiedNames)
        ~ " " ~ funcOrDelegStr ~ "("
        ~ prettyParamList!(str[1..1+demangleParamListAndRetValConsumed!(str[1..$])], wantQualifiedNames)
        ~ ")";
}

// Special case: types that are in function parameters
// For function parameters, the type can also contain 'lazy', 'out' or 'inout'.
template demangleFunctionParamType(char[] str, MangledNameType wantQualifiedNames)
{
    static if (str[0]=='L')
        const char [] demangleFunctionParamType = "lazy " ~ demangleType!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='K')
        const char [] demangleFunctionParamType = "inout " ~ demangleType!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='J')
        const char [] demangleFunctionParamType = "out " ~ demangleType!(str[1..$], wantQualifiedNames);
    else const char [] demangleFunctionParamType = demangleType!(str, wantQualifiedNames);
}

// Deal with 'out','inout', and 'lazy' parameters
template demangleFunctionParamTypeConsumed(char[] str)
{
    static if (str[0]=='K' || str[0]=='J' || str[0]=='L')
        const int demangleFunctionParamTypeConsumed = 1 + demangleTypeConsumed!(str[1..$]);
    else const int demangleFunctionParamTypeConsumed = demangleTypeConsumed!(str);
}

// Return true if c indicates a function. As well as 'F', it can be extern(Pascal), (C), (C++) or (Windows).
template isMangledFunction(char c)
{
    const bool isMangledFunction = (c=='F' || c=='U' || c=='W' || c=='V' || c=='R');
}

template prettyExtern(char c)
{
    static if (c=='F') const char [] prettyExtern = "";
    else static if (c=='U') const char [] prettyExtern = "extern (C) ";
    else static if (c=='W') const char [] prettyExtern = "extern (Windows) ";
    else static if (c=='V') const char [] prettyExtern = "extern (Pascal) ";
    else static if (c=='R') const char [] prettyExtern = "extern (C++) ";
    else static assert(0, "Unrecognized extern function.");
}

// Skip through the string until we find the return value. It can either be Z for normal
// functions, or Y for vararg functions, or X for lazy vararg functions.
template demangleReturnValue(char [] str, MangledNameType wantQualifiedNames)
{
    static assert(str.length>=1, "Demangle error(Function): No return value found");
    static if (str[0]=='Z' || str[0]=='Y' || str[0]=='X')
        const char[] demangleReturnValue = demangleType!(str[1..$], wantQualifiedNames);
    else const char [] demangleReturnValue = demangleReturnValue!(str[demangleFunctionParamTypeConsumed!(str)..$], wantQualifiedNames);
}

// Stop when we get to the return value
template prettyParamList(char [] str, MangledNameType wantQualifiedNames, char[] commastr = "")
{
    static if (str[0] == 'Z')
        const char [] prettyParamList = "";
    else static if (str[0] == 'Y')
        const char [] prettyParamList = commastr ~ "...";
    else static if (str[0]=='X') // lazy ...
        const char[] prettyParamList = commastr ~ "...";
    else
        const char [] prettyParamList =  commastr ~
            demangleFunctionParamType!(str[0..demangleFunctionParamTypeConsumed!(str)], wantQualifiedNames)
            ~ prettyParamList!(str[demangleFunctionParamTypeConsumed!(str)..$], wantQualifiedNames, ", ");
}

// How many characters are used in the parameter list and return value?
template demangleParamListAndRetValConsumed(char [] str)
{
    static assert (str.length>0, "Demangle error(ParamList): No return value found");
    static if (str[0]=='Z' || str[0]=='Y' || str[0]=='X')
        const int demangleParamListAndRetValConsumed = 1 + demangleTypeConsumed!(str[1..$]);
    else {
        const int demangleParamListAndRetValConsumed = demangleFunctionParamTypeConsumed!(str)
            + demangleParamListAndRetValConsumed!(str[demangleFunctionParamTypeConsumed!(str)..$]);
    }
}

// --------------------------------------------
//              TEMPLATES

// Pretty-print a template argument
template prettyTemplateArg(char [] str, MangledNameType wantQualifiedNames)
{
    static if (str[0]=='S') // symbol name
        const char [] prettyTemplateArg = prettyLname!(str[1..$], wantQualifiedNames);
    else static if (str[0]=='V') // value
        const char [] prettyTemplateArg =
            demangleType!(str[1..1+demangleTypeConsumed!(str[1..$])], wantQualifiedNames)
            ~ " = " ~ prettyValueArg!(str[1+demangleTypeConsumed!(str[1..$])..$]);
    else static if (str[0]=='T') // type
        const char [] prettyTemplateArg = demangleType!(str[1..$], wantQualifiedNames);

    else static assert(0, "Unrecognised template argument type: {" ~ str ~ "}");
}

template templateArgConsumed(char [] str)
{
    static if (str[0]=='S') // symbol name
        const int templateArgConsumed = 1 + getLnameConsumed!(str[1..$]);
    else static if (str[0]=='V') // value
        const int templateArgConsumed = 1 + demangleTypeConsumed!(str[1..$]) +
            templateValueArgConsumed!(str[1+demangleTypeConsumed!(str[1..$])..$]);
    else static if (str[0]=='T') // type
        const int templateArgConsumed = 1 + demangleTypeConsumed!(str[1..$]);
    else static assert(0, "Unrecognised template argument type: {" ~ str ~ "}");
}

// Return true if it is an entity which does not have D linkage
template isNonD_SymbolTemplateArg(char [] str)
{
    static if (str[0]!='S') const bool isNonD_SymbolTemplateArg = false;
    else static if (extractLname!(str[1..$]).length<2) const bool isNonD_SymbolTemplateArg = false;
    else const bool isNonD_SymbolTemplateArg = extractLname!(str[1..$])[0..2]!="_D";
}

// Like function parameter lists, template parameter lists also end in a Z,
// but they don't have a return value at the end.
template prettyTemplateArgList(char [] str, MangledNameType wantQualifiedNames, char [] commastr="")
{
    static if (str[0]=='Z')
        const char[] prettyTemplateArgList = "";
    else
       const char [] prettyTemplateArgList = commastr
            ~ prettyTemplateArg!(str[0..templateArgConsumed!(str)], wantQualifiedNames)
            ~ prettyTemplateArgList!(str[templateArgConsumed!(str)..$], wantQualifiedNames, ", ");
}

template templateArgListConsumed(char [] str)
{
    static assert(str.length>0, "No Z found at end of template argument list");
    static if (str[0]=='Z')
        const int templateArgListConsumed = 1;
    else
        const int templateArgListConsumed = templateArgConsumed!(str)
            + templateArgListConsumed!(str[templateArgConsumed!(str)..$]);
}

template templateFloatExponentConsumed(char [] str)
{
    static if (str[0]=='N')
        const int templateFloatExponentConsumed = 1 + countLeadingDigits!(str[1..$]);
    else const int templateFloatExponentConsumed = countLeadingDigits!(str);
}

template templateFloatArgConsumed(char [] str)
{
    static if (str.length>=3 && str[0..3]=="NAN") const int templateFloatArgConsumed = 3;
    else static if (str[0]=='N')
        const int templateFloatArgConsumed = 1 + templateFloatArgConsumed!(str[1..$]);
    else static if (str.length>=3 && str[0..3]=="INF") const int templateFloatArgConsumed = 3;
    else const int templateFloatArgConsumed = countLeadingHexDigits!(str) + 1 + templateFloatExponentConsumed!(str[countLeadingHexDigits!(str) + 1..$]);
}

template templateValueArgConsumed(char [] str)
{
    static if (str[0]=='n') const int templateValueArgConsumed = 1;
    else static if (beginsWithDigit!(str)) const int templateValueArgConsumed = countLeadingDigits!(str);
    else static if (str[0]=='N') const int templateValueArgConsumed = 1 + countLeadingDigits!(str[1..$]);
    else static if (str[0]=='e') const int templateValueArgConsumed = 1 + templateFloatArgConsumed!(str[1..$]);
    else static if (str[0]=='c') const int templateValueArgConsumed = 2 + templateFloatArgConsumed!(str[1..$]) + templateFloatArgConsumed!(str[2+ templateFloatArgConsumed!(str[1..$])..$]);
    else static assert(0, "Unknown character in template value argument:" ~ str);
}

// pretty-print a template value argument.
template prettyValueArg(char [] str)
{
    static if (str[0]=='n') const char [] prettyValueArg = "null";
    else static if (beginsWithDigit!(str)) const char [] prettyValueArg = str;
    else static if ( str[0]=='N') const char [] prettyValueArg = "-" ~ str[1..$];
    else static if ( str[0]=='e') const char [] prettyValueArg = prettyFloatValueArg!(str[1..$]);
    else static if ( str[0]=='c') const char [] prettyValueArg =
        prettyComplexArg!(str[1 .. 1+templateFloatArgConsumed!(str[1..$])],
                          str[2+templateFloatArgConsumed!(str[1..$]) .. $]);
    else const char [] prettyValueArg = "Value arg {" ~ str[0..$] ~ "}";
}

// --------------------------------------------
// Template float value arguments

private {

template prettyComplexArg(char [] realstr, char [] imagstr)
{
    static if (imagstr[0]=='N') {
        const char [] prettyComplexArg = prettyFloatValueArg!(realstr) ~ " - "
            ~ prettyFloatValueArg!(imagstr[1..$]) ~ "i";
    } else
    const char [] prettyComplexArg = prettyFloatValueArg!(realstr) ~ " + "
    ~ prettyFloatValueArg!(imagstr) ~ "i";
}

template prettyFloatExponent(char [] str)
{
    static if (str[0]=='N') const char [] prettyFloatExponent = "p-" ~ str[1..$];
    else const char [] prettyFloatExponent = "p+" ~ str;
}

// Display the floating point number in %a format (eg 0x1.ABCp-35);
template prettyFloatValueArg(char [] str)
{
    static if (str.length==3 && str=="NAN") const char [] prettyFloatValueArg="NaN";
    else static if (str[0]=='N') const char [] prettyFloatValueArg = "-" ~ prettyFloatValueArg!(str[1..$]);
    else static if (str.length==3 && str=="INF") const char [] prettyFloatValueArg="Inf";
    else const char [] prettyFloatValueArg = "0x" ~ str[0] ~ "." ~ str[1..countLeadingHexDigits!(str)]
       ~ prettyFloatExponent!(str[1+countLeadingHexDigits!(str)..$]);
}

}

// --------------------------------------------
//              UNIT TESTS

debug(UnitTest){

private {

const char [] THISFILE = "tango.util.meta.Demangle";

ireal SomeFunc(ushort u) { return -3i; }
idouble SomeFunc2(inout ushort u, ubyte w) { return -3i; }
byte[] SomeFunc3(out dchar d, ...) { return null; }
ifloat SomeFunc4(lazy void[] x...) { return 2i; }
char[dchar] SomeFunc5(lazy int delegate()[] z...);

extern (Windows) {
    typedef void function (double, long) WinFunc;
}
extern (Pascal) {
    typedef short[wchar] delegate (bool, ...) PascFunc;
}
extern (C) {
    typedef dchar delegate () CFunc;
}
extern (C++) {
    typedef cfloat function (wchar) CPPFunc;
}

interface SomeInterface {}

static assert( demangleType!((&SomeFunc).mangleof) == "ireal function (ushort)" );
static assert( demangleType!((&SomeFunc2).mangleof) == "idouble function (inout ushort, ubyte)");
static assert( demangleType!((&SomeFunc3).mangleof) == "byte[] function (out dchar, ...)");
static assert( demangleType!((&SomeFunc4).mangleof) == "ifloat function (lazy void[], ...)");
static assert( demangleType!((&SomeFunc5).mangleof) == "char[dchar] function (lazy int delegate ()[], ...)");
static assert( demangleType!((WinFunc).mangleof)== "extern (Windows) void function (double, long)");
static assert( demangleType!((PascFunc).mangleof) == "extern (Pascal) short[wchar] delegate (bool, ...)");
static assert( demangleType!((CFunc).mangleof) == "extern (C) dchar delegate ()");
static assert( demangleType!((CPPFunc).mangleof) == "extern (C++) cfloat function (wchar)");
// Interfaces are mangled as classes
static assert( demangleType!(SomeInterface.mangleof) == "class " ~ THISFILE ~ ".SomeInterface");


template ComplexTemplate(real a, creal b)
{
    class ComplexTemplate {}
}

// BUG: static assert( demangleType!((ComplexTemplate!(-0x1.23456789ABCDFFFEp-456, 0x1.12345p-16380L-3.2i)).mangleof) == "class " ~ THISFILE ~ ".ComplexTemplate!(double = -0x1.23456789ABCDFFFEp-456, creal = 0x1.12345p-16380 - 0x1.999999999999999Ap+1i).ComplexTemplate");
static assert( demangleType!((ComplexTemplate!(float.nan, -real.infinity+ireal.infinity)).mangleof) == "class " ~ THISFILE ~ ".ComplexTemplate!(float = NaN, creal = -Inf + Infi).ComplexTemplate");

}
}

