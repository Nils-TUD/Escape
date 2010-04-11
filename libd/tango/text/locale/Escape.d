module tango.text.locale.Escape;

version (Escape)
{
alias tango.text.locale.Escape nativeMethods;

private import tango.text.locale.Data;
private import tango.stdc.ctype;
private import tango.stdc.string;
private import tango.stdc.time;

int getUserCulture() {
  // TODO hardcoded until we have a locale
  char* env = "en_US.UTF-8";

  // getenv returns a string of the form <language>_<region>.
  // Therefore we need to replace underscores with hyphens.
  char* p = env;
  int len;
  while (*p) {
    if (*p == '.'){
      break;
    }
    if (*p == '_'){
      *p = '-';
    }
    p++;
    len++;
  }

  char[] s = env[0 .. len];
  foreach (entry; CultureData.cultureDataTable) {
    // todo: there is also a local compareString defined. Is it correct that here 
    // we use tango.text.locale.Data, which matches the signature?
    if (tango.text.locale.Data.compareString(entry.name, s) == 0)
	// todo: here was entry.id returned, which does not exist. Is lcid correct?
      return entry.lcid;
  }
  return 0;
}

void setUserCulture(int lcid) {
  // TODO atm ignored
}

ulong getUtcTime() {
   int t;
   time(&t);
   gmtime(&t);
   return cast(ulong)((cast(long)t * 10000000L) + 116444736000000000L);
}

short[] getDaylightChanges() {
  // TODO atm just dummy-data
  short[] data = new short[18];
  /+data[0] = cast(short)tzi.DaylightDate.wYear;
  data[1] = cast(short)tzi.DaylightDate.wMonth;
  data[2] = cast(short)tzi.DaylightDate.wDayOfWeek;
  data[3] = cast(short)tzi.DaylightDate.wDay;
  data[4] = cast(short)tzi.DaylightDate.wHour;
  data[5] = cast(short)tzi.DaylightDate.wMinute;
  data[6] = cast(short)tzi.DaylightDate.wSecond;
  data[7] = cast(short)tzi.DaylightDate.wMilliseconds;
  data[8] = cast(short)tzi.StandardDate.wYear;
  data[9] = cast(short)tzi.StandardDate.wMonth;
  data[10] = cast(short)tzi.StandardDate.wDayOfWeek;
  data[11] = cast(short)tzi.StandardDate.wDay;
  data[12] = cast(short)tzi.StandardDate.wHour;
  data[13] = cast(short)tzi.StandardDate.wMinute;
  data[14] = cast(short)tzi.StandardDate.wSecond;
  data[15] = cast(short)tzi.StandardDate.wMilliseconds;
  data[16] = cast(short)(tzi.DaylightBias * -1);
  data[17] = cast(short)(tzi.Bias * -1);+/
  return data;
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
