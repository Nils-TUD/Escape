/*******************************************************************************

        copyright:      Copyright (c) 2005 John Chapman. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: 2005

        author:         John Chapman

******************************************************************************/

module tango.text.locale.Escape;

version (Escape)
{
alias tango.text.locale.Escape nativeMethods;

private import tango.text.locale.Data;
private import tango.stdc.ctype;
private import tango.stdc.string;
private import tango.stdc.time;
private import tango.stdc.stringz : fromStringz;

int getUserCulture() {
  // TODO hardcoded until we have a locale
  char* env = "en_US.UTF-8";

  // getenv returns a string of the form <language>_<region>.
  // Therefore we need to replace underscores with hyphens.
  char[] s;
  if (env){
      s = fromStringz(env).dup;
      foreach (ref c; s)
               if (c == '.')
                   break;
               else
                  if (c == '_')
                      c = '-';
  } else {
      s="en-US";
  }
  foreach (entry; CultureData.cultureDataTable) {
    // todo: there is also a local compareString defined. Is it correct that here 
    // we use tango.text.locale.Data, which matches the signature?
    if (tango.text.locale.Data.compareString(entry.name, s) == 0)
      return entry.lcid;
  }
  
  foreach (entry; CultureData.cultureDataTable) {
    // todo: there is also a local compareString defined. Is it correct that here 
    // we use tango.text.locale.Data, which matches the signature?
    if (tango.text.locale.Data.compareString(entry.name, "en-US") == 0)
      return entry.lcid;
  }
  return 0;
}

void setUserCulture(int lcid) {
  // TODO atm ignored
}

int compareString(int lcid, char[] stringA, uint offsetA, uint lengthA, char[] stringB, uint offsetB, uint lengthB, bool ignoreCase) {
  // TODO atm locale is ignored here
  
  void strToLower(char[] string) {
    for(int i = 0; i < string.length; i++) {
      string[i] = cast(char)(tolower(cast(int)string[i]));
    }
  }

  char[] s1 = stringA[offsetA..offsetA+lengthA].dup,
         s2 = stringB[offsetB..offsetB+lengthB].dup;
  if(ignoreCase) {
    strToLower(s1);
    strToLower(s2);
  }
  
  return strcmp(s1[offsetA..offsetA+lengthA].ptr, s2[offsetB..offsetB+lengthB].ptr);
}

debug(UnitTest)
{
    unittest
    {
        int c = getUserCulture();
        assert(compareString(c, "Alphabet", 0, 8, "Alphabet", 0, 8, false) == 0);
        assert(compareString(c, "Alphabet", 0, 8, "alphabet", 0, 8, true) == 0);
        assert(compareString(c, "Alphabet", 0, 8, "alphabet", 0, 8, false) != 0);
        assert(compareString(c, "lphabet", 0, 7, "alphabet", 0, 8, true) != 0);
        assert(compareString(c, "Alphabet", 0, 8, "lphabet", 0, 7, true) != 0);
    }
}
}
