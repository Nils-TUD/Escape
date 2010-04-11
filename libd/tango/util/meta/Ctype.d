/**
 * Simple ASCII character classification functions.
 *
 * Compile-time equivalents of stdc.ctype
 * All these functions return false if presented with a non-ASCII character.
 *
 * License:   BSD style: $(LICENSE)
 * Authors:   Don Clugston
 * Copyright: Copyright (C) 2005-2006 Don Clugston
 */
module tango.util.meta.Ctype;
// Naive implementation. This works fine, because it is only executed at compile time.

template isalnum(dchar c)
{
  static if (isalpha!(c) || isdigit!(c))
     const bool isalnum=true;
  else const bool isalnum=false;
}

template isalpha(dchar c)
{
  static if ((c>='A' && c<='Z') || (c>='a' && c<='z'))
     const bool isalpha=true;
  else const bool isalpha=false;
}

template iscntrl(dchar c)
{
  static if (c<0x20 || c==0x7F) const bool iscntrl=true;
  else const bool iscntrl = false;
}

template isdigit(dchar c)
{
  static if (c>='0' && c<='9') const bool isdigit=true;
  else const bool isdigit = false;
}

template isspace(dchar c)
{
  static if (c==' ' || (c>=0xA && c<=0xD) ) const bool isspace=true;
  else const bool isspace=false;
}

template isupper(dchar c)
{
  static if (c>='A' && c<='Z')  const bool isupper=true;
  else const bool isupper=false;
}

template islower(dchar c)
{
  static if (c>='a' && c<='z')  const bool islower=true;
  else const bool islower=false;
}

template ispunct(dchar c)
{
  static if (c>' ' && c<0x7F && !isdigit!(c) && !isalpha!(c) ) const bool ispunct=true;
  else const bool ispunct=false;
}

template isxdigit(dchar c)
{
  static if (isdigit!(c) || (c>='a' && c<='f') || (c>='A' && c<='F')) const bool isxdigit=true;
  else const bool isxdigit=false;
}

template isgraph(dchar c)
{
  static if (c>' ' && c<0x7F) const bool isgraph=true;
  else const bool isgraph=false;
}

template isprint(dchar c)
{
  static if (c>=' ' && c<0x7F) const bool isprint=true;
  else const bool isprint=false;
}

template isascii(dchar c)
{
  static if (c<=0x7F) const bool isascii=true;
  else const bool isascii=false;
}

template toupper(dchar c)
{
  static if (islower!(c)) const dchar toupper = c + 'A'-'a';
  else const dchar toupper = c;
}

template tolower(dchar c)
{
  static if (c>='A' && c<='Z') const dchar tolower = c + 'a'-'A';
  else const dchar tolower = c;
}

debug(UnitTest) {
  static assert(isspace!(' '));
  static assert(!isspace!('w'));
  static assert(islower!('r'));
  static assert(toupper!('f')=='F');
  static assert(tolower!('Z')=='z');
  static assert(isdigit!('4'));
  static assert(!isxdigit!('G'));
  static assert(ispunct!('@'));
  static assert(isupper!('A'));
}