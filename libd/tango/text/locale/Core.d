/**
 * Contains classes that provide information about locales, such as the language and calendars, 
 * as well as cultural conventions used for formatting dates, currency and numbers. Use these classes when writing
 * applications for an international audience.
 *
 * $(MEMBERTABLE
 * $(TR
 * $(TH Interface)
 * $(TH Description)
 * )
 * $(TR
 * $(TD $(LINK2 #IFormatService, IFormatService))
 * $(TD Retrieves an object to control formatting.)
 * )
 * )
 *
 * $(MEMBERTABLE
 * $(TR
 * $(TH Class)
 * $(TH Description)
 * )
 * $(TR
 * $(TD $(LINK2 #Calendar, Calendar))
 * $(TD Represents time in week, month and year divisions.)
 * )
 * $(TR
 * $(TD $(LINK2 #Culture, Culture))
 * $(TD Provides information about a culture, such as its name, calendar and date and number format patterns.)
 * )
 * $(TR
 * $(TD $(LINK2 #DateTimeFormat, DateTimeFormat))
 * $(TD Determines how $(LINK2 #DateTime, DateTime) values are formatted, depending on the culture.)
 * )
 * $(TR
 * $(TD $(LINK2 #DaylightSavingTime, DaylightSavingTime))
 * $(TD Represents a period of daylight-saving time.)
 * )
 * $(TR
 * $(TD $(LINK2 #GregorianCalendar, GregorianCalendar))
 * $(TD Represents the Gregorian calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #HebrewCalendar, HebrewCalendar))
 * $(TD Represents the Hebrew calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #HijriCalendar, HijriCalendar))
 * $(TD Represents the Hijri calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #JapaneseCalendar, JapaneseCalendar))
 * $(TD Represents the Japanese calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #KoreanCalendar, KoreanCalendar))
 * $(TD Represents the Korean calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #NumberFormat, NumberFormat))
 * $(TD Determines how numbers are formatted, according to the current culture.)
 * )
 * $(TR
 * $(TD $(LINK2 #Region, Region))
 * $(TD Provides information about a region.)
 * )
 * $(TR
 * $(TD $(LINK2 #TaiwanCalendar, TaiwanCalendar))
 * $(TD Represents the Taiwan calendar.)
 * )
 * $(TR
 * $(TD $(LINK2 #ThaiBuddhistCalendar, ThaiBuddhistCalendar))
 * $(TD Represents the Thai Buddhist calendar.)
 * )
 * )
 *
 * $(MEMBERTABLE
 * $(TR
 * $(TH Struct)
 * $(TH Description)
 * )
 * $(TR
 * $(TD $(LINK2 #DateTime, DateTime))
 * $(TD Represents time expressed as a date and time of day.)
 * )
 * $(TR
 * $(TD $(LINK2 #TimeSpan, TimeSpan))
 * $(TD Represents a time interval.)
 * )
 * )
 */
module tango.text.locale.Core;

private import  tango.text.locale.Constants,
                tango.text.locale.Data,
                tango.text.locale.Format,
                tango.text.locale.Parse;

version (Windows)
  private import tango.text.locale.Win32;
else version (Posix)
  private import tango.text.locale.Linux;
else version (Escape)
  private import tango.text.locale.Escape;

// Used by cloneObject.
extern (C) 
private Object _d_newclass(ClassInfo info);

// Creates a shallow copy of an object.
private Object cloneObject(Object obj) {
  if (obj is null)
    return null;

  ClassInfo ci = obj.classinfo;
  size_t start = Object.classinfo.init.length;
  size_t end = ci.init.length;

  Object clone = _d_newclass(ci);
  (cast(void*)clone)[start .. end] = (cast(void*)obj)[start .. end];
  return clone;
}

// Initializes an array.
template arrayOf(T) {
  private T[] arrayOf(T[] params ...) {
    return params.dup;
  }
}

/**
 * $(ANCHOR _IFormatService)
 * Retrieves an object to control formatting.
 * 
 * A class implements $(LINK2 #IFormatService_getFormat, getFormat) to retrieve an object that provides format information for the implementing type.
 * Remarks: IFormatService is implemented by $(LINK2 #Culture, Culture), $(LINK2 #NumberFormat, NumberFormat) and $(LINK2 #DateTimeFormat, DateTimeFormat) to provide locale-specific formatting of
 * numbers and date and time values.
 */
public interface IFormatService {

  /**
   * $(ANCHOR IFormatService_getFormat)
   * Retrieves an object that supports formatting for the specified _type.
   * Returns: The current instance if type is the same _type as the current instance; otherwise, null.
   * Params: type = An object that specifies the _type of formatting to retrieve.
   */
  Object getFormat(TypeInfo type);

}

/**
 * $(ANCHOR _Culture)
 * Provides information about a culture, such as its name, calendar and date and number format patterns.
 * Remarks: tango.text.locale adopts the RFC 1766 standard for culture names in the format &lt;language&gt;"-"&lt;region&gt;. 
 * &lt;language&gt; is a lower-case two-letter code defined by ISO 639-1. &lt;region&gt; is an upper-case 
 * two-letter code defined by ISO 3166. For example, "en-GB" is UK English.
 * $(BR)$(BR)There are three types of culture: invariant, neutral and specific. The invariant culture is not tied to
 * any specific region, although it is associated with the English language. A neutral culture is associated with
 * a language, but not with a region. A specific culture is associated with a language and a region. "es" is a neutral 
 * culture. "es-MX" is a specific culture.
 * $(BR)$(BR)Instances of $(LINK2 #DateTimeFormat, DateTimeFormat) and $(LINK2 #NumberFormat, NumberFormat) cannot be created for neutral cultures.
 * Examples:
 * ---
 * import tango.io.Stdout, tango.text.locale.Common;
 *
 * void main() {
 *   Culture culture = new Culture("it-IT");
 *
 *   Stdout.format("englishName: {0}\n", culture.englishName);
 *   Stdout.format("nativeName: {0}\n", culture.nativeName);
 *   Stdout.format("name: {0}\n", culture.name);
 *   Stdout.format("parent: {0}\n", culture.parent.name);
 *   Stdout.format("isNeutral: {0}\n", culture.isNeutral);
 * }
 *
 * // Produces the following output:
 * // englishName: Italian (Italy)
 * // nativeName: italiano (Italia)
 * // name: it-IT
 * // parent: it
 * // isNeutral: false
 * ---
 */
public class Culture : IFormatService {

  private const int LCID_INVARIANT = 0x007F;

  private static Culture[char[]] namedCultures;
  private static Culture[int] idCultures;
  private static Culture[char[]] ietfCultures;

  private static Culture currentCulture_;
  private static Culture userDefaultCulture_; // The user's default culture (GetUserDefaultLCID).
  private static Culture invariantCulture_; // The invariant culture is associated with the English language.
  private Calendar calendar_;
  private Culture parent_;
  private CultureData* cultureData_;
  private bool isReadOnly_;
  private NumberFormat numberFormat_;
  private DateTimeFormat dateTimeFormat_;

  static this() {
    invariantCulture_ = new Culture(LCID_INVARIANT);
    invariantCulture_.isReadOnly_ = true;

    userDefaultCulture_ = new Culture(nativeMethods.getUserCulture());
    if (userDefaultCulture_ is null)
      // Fallback
      userDefaultCulture_ = invariantCulture;
    else
      userDefaultCulture_.isReadOnly_ = true;
  }

  static ~this() {
    namedCultures = null;
    idCultures = null;
    ietfCultures = null;
  }

  /**
   * Initializes a new Culture instance from the supplied name.
   * Params: cultureName = The name of the Culture.
   */
  public this(char[] cultureName) {
    cultureData_ = CultureData.getDataFromCultureName(cultureName);
  }

  /**
   * Initializes a new Culture instance from the supplied culture identifier.
   * Params: cultureID = The identifer (LCID) of the Culture.
   * Remarks: Culture identifiers correspond to a Windows LCID.
   */
  public this(int cultureID) {
    cultureData_ = CultureData.getDataFromCultureID(cultureID);
  }

  /**
   * Retrieves an object defining how to format the specified type.
   * Params: type = The TypeInfo of the resulting formatting object.
   * Returns: If type is typeid($(LINK2 #NumberFormat, NumberFormat)), the value of the $(LINK2 #Culture_numberFormat, numberFormat) property. If type is typeid($(LINK2 #DateTimeFormat, DateTimeFormat)), the
   * value of the $(LINK2 #Culture_dateTimeFormat, dateTimeFormat) property. Otherwise, null.
   * Remarks: Implements $(LINK2 #IFormatService_getFormat, IFormatService.getFormat).
   */
  public Object getFormat(TypeInfo type) {
    if (type is typeid(NumberFormat))
      return numberFormat;
    else if (type is typeid(DateTimeFormat))
      return dateTimeFormat;
    return null;
  }

  /**
   * Copies the current Culture instance.
   * Returns: A copy of the current Culture instance.
   * Remarks: The values of the $(LINK2 #Culture_numberFormat, numberFormat), $(LINK2 #Culture_dateTimeFormat, dateTimeFormat) and $(LINK2 #Culture_calendar, calendar) properties are copied also.
   */
  public Object clone() {
    Culture culture = cast(Culture)cloneObject(this);
    if (!culture.isNeutral) {
      if (dateTimeFormat_ !is null)
        culture.dateTimeFormat_ = cast(DateTimeFormat)dateTimeFormat_.clone();
      if (numberFormat_ !is null)
        culture.numberFormat_ = cast(NumberFormat)numberFormat_.clone();
    }
    if (calendar_ !is null)
      culture.calendar_ = cast(Calendar)calendar_.clone();
    return culture;
  }

  /**
   * Returns a read-only instance of a culture using the specified culture identifier.
   * Params: cultureID = The identifier of the culture.
   * Returns: A read-only culture instance.
   * Remarks: Instances returned by this method are cached.
   */
  public static Culture getCulture(int cultureID) {
    Culture culture = getCultureInternal(cultureID, null);
    if (culture is null)
      throw new Exception("Culture is not supported.");
    return culture;
  }

  /**
   * Returns a read-only instance of a culture using the specified culture name.
   * Params: cultureName = The name of the culture.
   * Returns: A read-only culture instance.
   * Remarks: Instances returned by this method are cached.
   */
  public static Culture getCulture(char[] cultureName) {
    if (cultureName == null)
      throw new Exception("Value cannot be null.");
    Culture culture = getCultureInternal(0, cultureName);
    if (culture is null)
      throw new Exception("Culture name " ~ cultureName ~ " is not supported.");
    return culture;
  }

  /**
    * Returns a read-only instance using the specified name, as defined by the RFC 3066 standard and maintained by the IETF.
    * Params: name = The name of the language.
    * Returns: A read-only culture instance.
    */
  public static Culture getCultureFromIetfLanguageTag(char[] name) {
    if (name == null)
      throw new Exception("Value cannot be null.");
    Culture culture = getCultureInternal(-1, name);
    if (culture is null)
      throw new Exception("Culture IETF name " ~ name ~ " is not a known IETF name.");
    return culture;
  }

  private static Culture getCultureInternal(int cultureID, char[] name) {
    // If cultureID is - 1, name is an IETF name; if it's 0, name is a culture name; otherwise, it's a valid LCID.

    // Look up tables first.
    if (cultureID == 0) {
      if (Culture* culture = name in namedCultures)
        return *culture;
    }
    else if (cultureID > 0) {
      if (Culture* culture = cultureID in idCultures)
        return *culture;
    }
    else if (cultureID == -1) {
      if (Culture* culture = name in ietfCultures)
        return *culture;
    }

    // Nothing found, create a new instance.
    Culture culture;

    try {
      if (cultureID == -1) {
        name = CultureData.getCultureNameFromIetfName(name);
        if (name == null)
          return null;
      }
      else if (cultureID == 0)
        culture = new Culture(name);
      else if (userDefaultCulture_ !is null && userDefaultCulture_.id == cultureID) {
        culture = userDefaultCulture_;
      }
      else
        culture = new Culture(cultureID);
    }
    catch (Exception) {
      return null;
    }

    culture.isReadOnly_ = true;

    // Now cache the new instance in all tables.
    ietfCultures[culture.ietfLanguageTag] = culture;
    namedCultures[culture.name] = culture;
    idCultures[culture.id] = culture;

    return culture;
  }

  /**
   * Returns a list of cultures filtered by the specified $(LINK2 constants.html#CultureTypes, CultureTypes).
   * Params: types = A combination of CultureTypes.
   * Returns: An array of Culture instances containing cultures specified by types.
   */
  public static Culture[] getCultures(CultureTypes types) {
    bool includeSpecific = (types & CultureTypes.Specific) != 0;
    bool includeNeutral = (types & CultureTypes.Neutral) != 0;

    int[] cultures;
    for (int i = 0; i < CultureData.cultureDataTable.length; i++) {
      if ((CultureData.cultureDataTable[i].isNeutral && includeNeutral) || (!CultureData.cultureDataTable[i].isNeutral && includeSpecific))
        cultures ~= CultureData.cultureDataTable[i].lcid;
    }

    Culture[] result = new Culture[cultures.length];
    foreach (int i, int cultureID; cultures)
      result[i] = new Culture(cultureID);
    return result;
  }

  /**
   * Returns the name of the Culture.
   * Returns: A string containing the name of the Culture in the format &lt;language&gt;"-"&lt;region&gt;.
   */
  public override char[] toUtf8() {
    return cultureData_.name;
  }

  public override int opEquals(Object obj) {
    if (obj is this)
      return true;
    Culture other = cast(Culture)obj;
    if (other is null)
      return false;
    return other.name == name; // This needs to be changed so it's culturally aware.
  }

  /**
   * $(ANCHOR Culture_current)
   * $(I Property.) Retrieves the culture of the current user.
   * Returns: The Culture instance representing the user's current culture.
   */
  public static Culture current() {
    if (currentCulture_ !is null)
      return currentCulture_;

    if (userDefaultCulture_ !is null) {
      // If the user has changed their locale settings since last we checked, invalidate our data.
      if (userDefaultCulture_.id != nativeMethods.getUserCulture())
        userDefaultCulture_ = null;
    }
    if (userDefaultCulture_ is null) {
      userDefaultCulture_ = new Culture(nativeMethods.getUserCulture());
      if (userDefaultCulture_ is null)
        userDefaultCulture_ = invariantCulture;
      else
        userDefaultCulture_.isReadOnly_ = true;
    }

    return userDefaultCulture_;
  }
  /**
   * $(I Property.) Assigns the culture of the _current user.
   * Params: value = The Culture instance representing the user's _current culture.
   * Examples:
   * The following examples shows how to change the _current culture.
   * ---
   * import tango.io.Print, tango.text.locale.Common;
   *
   * void main() {
   *   // Displays the name of the current culture.
   *   Println("The current culture is %s.", Culture.current.englishName);
   *
   *   // Changes the current culture to el-GR.
   *   Culture.current = new Culture("el-GR");
   *   Println("The current culture is now %s.", Culture.current.englishName);
   * }
   *
   * // Produces the following output:
   * // The current culture is English (United Kingdom).
   * // The current culture is now Greek (Greece).
   * ---
   */
  public static void current(Culture value) {
    checkNeutral(value);
    nativeMethods.setUserCulture(value.id);
    currentCulture_ = value;
  }

  /**
   * $(I Property.) Retrieves the invariant Culture.
   * Returns: The Culture instance that is invariant.
   * Remarks: The invariant culture is culture-independent. It is not tied to any specific region, but is associated
   * with the English language.
   */
  public static Culture invariantCulture() {
    return invariantCulture_;
  }

  /**
   * $(I Property.) Retrieves the identifier of the Culture.
   * Returns: The culture identifier of the current instance.
   * Remarks: The culture identifier corresponds to the Windows locale identifier (LCID). It can therefore be used when 
   * interfacing with the Windows NLS functions.
   */
  public int id() {
    return cultureData_.lcid;
  }

  /**
   * $(ANCHOR Culture_name)
   * $(I Property.) Retrieves the name of the Culture in the format &lt;language&gt;"-"&lt;region&gt;.
   * Returns: The name of the current instance. For example, the name of the UK English culture is "en-GB".
   */
  public char[] name() {
    return cultureData_.name;
  }

  /**
   * $(I Property.) Retrieves the name of the Culture in the format &lt;languagename&gt; (&lt;regionname&gt;) in English.
   * Returns: The name of the current instance in English. For example, the englishName of the UK English culture 
   * is "English (United Kingdom)".
   */
  public char[] englishName() {
    return cultureData_.englishName;
  }

  /**
   * $(I Property.) Retrieves the name of the Culture in the format &lt;languagename&gt; (&lt;regionname&gt;) in its native language.
   * Returns: The name of the current instance in its native language. For example, if Culture.name is "de-DE", nativeName is 
   * "Deutsch (Deutschland)".
   */
  public char[] nativeName() {
    return cultureData_.nativeName;
  }

  /**
   * $(I Property.) Retrieves the two-letter language code of the culture.
   * Returns: The two-letter language code of the Culture instance. For example, the twoLetterLanguageName for English is "en".
   */
  public char[] twoLetterLanguageName() {
    return cultureData_.isoLangName;
  }

  /**
   * $(I Property.) Retrieves the three-letter language code of the culture.
   * Returns: The three-letter language code of the Culture instance. For example, the threeLetterLanguageName for English is "eng".
   */
  public char[] threeLetterLanguageName() {
    return cultureData_.isoLangName2;
  }

  /**
   * $(I Property.) Retrieves the RFC 3066 identification for a language.
   * Returns: A string representing the RFC 3066 language identification.
   */
  public final char[] ietfLanguageTag() {
    return cultureData_.ietfTag;
  }

  /**
   * $(I Property.) Retrieves the Culture representing the parent of the current instance.
   * Returns: The Culture representing the parent of the current instance.
   */
  public Culture parent() {
    if (parent_ is null) {
      try {
        int parentCulture = cultureData_.parent;
        if (parentCulture == LCID_INVARIANT)
          parent_ = invariantCulture;
        else
          parent_ = new Culture(parentCulture);
      }
      catch {
        parent_ = invariantCulture;
      }
    }
    return parent_;
  }

  /**
   * $(I Property.) Retrieves a value indicating whether the current instance is a neutral culture.
   * Returns: true is the current Culture represents a neutral culture; otherwise, false.
   * Examples:
   * The following example displays which cultures using Chinese are neutral.
   * ---
   * import tango.io.Print, tango.text.locale.Common;
   *
   * void main() {
   *   foreach (c; Culture.getCultures(CultureTypes.All)) {
   *     if (c.twoLetterLanguageName == "zh") {
   *       Print(c.englishName);
   *       if (c.isNeutral)
   *         Println("neutral");
   *       else
   *         Println("specific");
   *     }
   *   }
   * }
   *
   * // Produces the following output:
   * // Chinese (Simplified) - neutral
   * // Chinese (Taiwan) - specific
   * // Chinese (People's Republic of China) - specific
   * // Chinese (Hong Kong S.A.R.) - specific
   * // Chinese (Singapore) - specific
   * // Chinese (Macao S.A.R.) - specific
   * // Chinese (Traditional) - neutral
   * ---
   */
  public bool isNeutral() {
    return cultureData_.isNeutral;
  }

  /**
   * $(I Property.) Retrieves a value indicating whether the instance is read-only.
   * Returns: true if the instance is read-only; otherwise, false.
   * Remarks: If the culture is read-only, the $(LINK2 #Culture_dateTimeFormat, dateTimeFormat) and $(LINK2 #Culture_numberFormat, numberFormat) properties return 
   * read-only instances.
   */
  public final bool isReadOnly() {
    return isReadOnly_;
  }

  /**
   * $(ANCHOR Culture_calendar)
   * $(I Property.) Retrieves the calendar used by the culture.
   * Returns: A Calendar instance respresenting the calendar used by the culture.
   */
  public Calendar calendar() {
    if (calendar_ is null) {
      calendar_ = getCalendarInstance(cultureData_.calendarType);
      calendar_.isReadOnly_ = isReadOnly_;
    }
    return calendar_;
  }

  /**
   * $(I Property.) Retrieves the list of calendars that can be used by the culture.
   * Returns: An array of type Calendar representing the calendars that can be used by the culture.
   */
  public Calendar[] optionalCalendars() {
    Calendar[] cals = new Calendar[cultureData_.optionalCalendars.length];
    foreach (int i, int calID; cultureData_.optionalCalendars)
      cals[i] = getCalendarInstance(calID);
    return cals;
  }

  /**
   * $(ANCHOR Culture_numberFormat)
   * $(I Property.) Retrieves a NumberFormat defining the culturally appropriate format for displaying numbers and currency.
   * Returns: A NumberFormat defining the culturally appropriate format for displaying numbers and currency.
  */
  public NumberFormat numberFormat() {
    checkNeutral(this);
    if (numberFormat_ is null) {
      numberFormat_ = new NumberFormat(cultureData_);
      numberFormat_.isReadOnly_ = isReadOnly_;
    }
    return numberFormat_;
  }
  /**
   * $(I Property.) Assigns a NumberFormat defining the culturally appropriate format for displaying numbers and currency.
   * Params: values = A NumberFormat defining the culturally appropriate format for displaying numbers and currency.
   */
  public void numberFormat(NumberFormat value) {
    checkReadOnly();
    numberFormat_ = value;
  }

  /**
   * $(ANCHOR Culture_dateTimeFormat)
   * $(I Property.) Retrieves a DateTimeFormat defining the culturally appropriate format for displaying dates and times.
   * Returns: A DateTimeFormat defining the culturally appropriate format for displaying dates and times.
   */
  public DateTimeFormat dateTimeFormat() {
    checkNeutral(this);
    if (dateTimeFormat_ is null) {
      dateTimeFormat_ = new DateTimeFormat(cultureData_, calendar);
      dateTimeFormat_.isReadOnly_ = isReadOnly_;
    }
    return dateTimeFormat_;
  }
  /**
   * $(I Property.) Assigns a DateTimeFormat defining the culturally appropriate format for displaying dates and times.
   * Params: values = A DateTimeFormat defining the culturally appropriate format for displaying dates and times.
   */
  public void dateTimeFormat(DateTimeFormat value) {
    checkReadOnly();
    dateTimeFormat_ = value;
  }

  private static void checkNeutral(Culture culture) {
    if (culture.isNeutral)
      throw new Exception("Culture '" ~ culture.name ~ "' is a neutral culture. It cannot be used in formatting and therefore cannot be set as the current culture.");
  }

  private void checkReadOnly() {
    if (isReadOnly_)
      throw new Exception("Instance is read-only.");
  }

  private static Calendar getCalendarInstance(int calendarType) {
    switch (calendarType) {
      case Calendar.JAPAN:
        return new JapaneseCalendar;
      case Calendar.TAIWAN:
        return new TaiwanCalendar;
      case Calendar.KOREA:
        return new KoreanCalendar;
      case Calendar.HIJRI:
        return new HijriCalendar;
      case Calendar.THAI:
        return new ThaiBuddhistCalendar;
      case Calendar.HEBREW:
        return new HebrewCalendar;
      case Calendar.GREGORIAN_US:
      case Calendar.GREGORIAN_ME_FRENCH:
      case Calendar.GREGORIAN_ARABIC:
      case Calendar.GREGORIAN_XLIT_ENGLISH:
      case Calendar.GREGORIAN_XLIT_FRENCH:
        return new GregorianCalendar(cast(GregorianCalendarTypes)calendarType);
      default:
        break;
    }
    return new GregorianCalendar;
  }

}

/**
 * $(ANCHOR _Region)
 * Provides information about a region.
 * Remarks: Region does not represent user preferences. It does not depend on the user's language or culture.
 * Examples:
 * The following example displays some of the properties of the Region class:
 * ---
 * import tango.io.Print, tango.text.locale.Common;
 *
 * void main() {
 *   Region region = new Region("en-GB");
 *   Println("name:              %s", region.name);
 *   Println("englishName:       %s", region.englishName);
 *   Println("isMetric:          %s", region.isMetric);
 *   Println("currencySymbol:    %s", region.currencySymbol);
 *   Println("isoCurrencySymbol: %s", region.isoCurrencySymbol);
 * }
 *
 * // Produces the following output.
 * // name:              en-GB
 * // englishName:       United Kingdom
 * // isMetric:          true
 * // currencySymbol:    £
 * // isoCurrencySymbol: GBP
 * ---
 */
public class Region {

  private CultureData* cultureData_;
  private static Region currentRegion_;
  private char[] name_;

  /**
   * Initializes a new Region instance based on the region associated with the specified culture identifier.
   * Params: cultureID = A culture indentifier.
   * Remarks: The name of the Region instance is set to the ISO 3166 two-letter code for that region.
   */
  public this(int cultureID) {
    cultureData_ = CultureData.getDataFromCultureID(cultureID);
    if (cultureData_.isNeutral)
      throw new Exception("Cannot use a neutral culture to create a region.");
    name_ = cultureData_.regionName;
  }

  /**
   * $(ANCHOR Region_ctor_name)
   * Initializes a new Region instance based on the region specified by name.
   * Params: name = A two-letter ISO 3166 code for the region. Or, a culture $(LINK2 #Culture_name, _name) consisting of the language and region.
   */
  public this(char[] name) {
    cultureData_ = CultureData.getDataFromRegionName(name);
    name_ = name;
    if (cultureData_.isNeutral)
      throw new Exception("The region name " ~ name ~ " corresponds to a neutral culture and cannot be used to create a region.");
  }

  package this(CultureData* cultureData) {
    cultureData_ = cultureData;
    name_ = cultureData.regionName;
  }

  /**
   * $(I Property.) Retrieves the Region used by the current $(LINK2 #Culture, Culture).
   * Returns: The Region instance associated with the current Culture.
   */
  public static Region current() {
    if (currentRegion_ is null)
      currentRegion_ = new Region(Culture.current.cultureData_);
    return currentRegion_;
  }

  /**
   * $(I Property.) Retrieves a unique identifier for the geographical location of the region.
   * Returns: An $(B int) uniquely identifying the geographical location.
   */
  public int geoID() {
    return cultureData_.geoId;
  }

  /**
   * $(ANCHOR Region_name)
   * $(I Property.) Retrieves the ISO 3166 code, or the name, of the current Region.
   * Returns: The value specified by the name parameter of the $(LINK2 #Region_ctor_name, Region(char[])) constructor.
   */
  public char[] name() {
    return name_;
  }

  /**
   * $(I Property.) Retrieves the full name of the region in English.
   * Returns: The full name of the region in English.
   */
  public char[] englishName() {
    return cultureData_.englishCountry;
  }

  /**
   * $(I Property.) Retrieves the full name of the region in its native language.
   * Returns: The full name of the region in the language associated with the region code.
   */
  public char[] nativeName() {
    return cultureData_.nativeCountry;
  }

  /**
   * $(I Property.) Retrieves the two-letter ISO 3166 code of the region.
   * Returns: The two-letter ISO 3166 code of the region.
   */
  public char[] twoLetterRegionName() {
    return cultureData_.regionName;
  }

  /**
   * $(I Property.) Retrieves the three-letter ISO 3166 code of the region.
   * Returns: The three-letter ISO 3166 code of the region.
   */
  public char[] threeLetterRegionName() {
    return cultureData_.isoRegionName;
  }

  /**
   * $(I Property.) Retrieves the currency symbol of the region.
   * Returns: The currency symbol of the region.
   */
  public char[] currencySymbol() {
    return cultureData_.currency;
  }

  /**
   * $(I Property.) Retrieves the three-character currency symbol of the region.
   * Returns: The three-character currency symbol of the region.
   */
  public char[] isoCurrencySymbol() {
    return cultureData_.intlSymbol;
  }

  /**
   * $(I Property.) Retrieves the name in English of the currency used in the region.
   * Returns: The name in English of the currency used in the region.
   */
  public char[] currencyEnglishName() {
    return cultureData_.englishCurrency;
  }

  /**
   * $(I Property.) Retrieves the name in the native language of the region of the currency used in the region.
   * Returns: The name in the native language of the region of the currency used in the region.
   */
  public char[] currencyNativeName() {
    return cultureData_.nativeCurrency;
  }

  /**
   * $(I Property.) Retrieves a value indicating whether the region uses the metric system for measurements.
   * Returns: true is the region uses the metric system; otherwise, false.
   */
  public bool isMetric() {
    return cultureData_.isMetric;
  }

  /**
   * Returns a string containing the ISO 3166 code, or the $(LINK2 #Region_name, name), of the current Region.
   * Returns: A string containing the ISO 3166 code, or the name, of the current Region.
   */
  public override char[] toUtf8() {
    return name_;
  }

}

/**
 * $(ANCHOR _NumberFormat)
 * Determines how numbers are formatted, according to the current culture.
 * Remarks: Numbers are formatted using format patterns retrieved from a NumberFormat instance.
 * This class implements $(LINK2 #IFormatService_getFormat, IFormatService.getFormat).
 * Examples:
 * The following example shows how to retrieve an instance of NumberFormat for a Culture
 * and use it to display number formatting information.
 * ---
 * import tango.io.Print, tango.text.locale.Common;
 *
 * void main(char[][] args) {
 *   foreach (c; Culture.getCultures(CultureTypes.Specific)) {
 *     if (c.twoLetterLanguageName == "en") {
 *       NumberFormat fmt = c.numberFormat;
 *       Println("The currency symbol for %s is '%s'", 
 *         c.englishName, 
 *         fmt.currencySymbol);
 *     }
 *   }
 * }
 *
 * // Produces the following output:
 * // The currency symbol for English (United States) is '$'
 * // The currency symbol for English (United Kingdom) is '£'
 * // The currency symbol for English (Australia) is '$'
 * // The currency symbol for English (Canada) is '$'
 * // The currency symbol for English (New Zealand) is '$'
 * // The currency symbol for English (Ireland) is '€'
 * // The currency symbol for English (South Africa) is 'R'
 * // The currency symbol for English (Jamaica) is 'J$'
 * // The currency symbol for English (Caribbean) is '$'
 * // The currency symbol for English (Belize) is 'BZ$'
 * // The currency symbol for English (Trinidad and Tobago) is 'TT$'
 * // The currency symbol for English (Zimbabwe) is 'Z$'
 * // The currency symbol for English (Republic of the Philippines) is 'Php'
 *---
 */
public class NumberFormat : IFormatService {

  package bool isReadOnly_;
  private static NumberFormat invariantFormat_;

  private int numberDecimalDigits_;
  private int numberNegativePattern_;
  private int currencyDecimalDigits_;
  private int currencyNegativePattern_;
  private int currencyPositivePattern_;
  private int[] numberGroupSizes_;
  private int[] currencyGroupSizes_;
  private char[] numberGroupSeparator_;
  private char[] numberDecimalSeparator_;
  private char[] currencyGroupSeparator_;
  private char[] currencyDecimalSeparator_;
  private char[] currencySymbol_;
  private char[] negativeSign_;
  private char[] positiveSign_;
  private char[] nanSymbol_;
  private char[] negativeInfinitySymbol_;
  private char[] positiveInfinitySymbol_;
  private char[][] nativeDigits_;

  /**
   * Initializes a new, culturally independent instance.
   *
   * Remarks: Modify the properties of the new instance to define custom formatting.
   */
  public this() {
    this(null);
  }

  package this(CultureData* cultureData) {
    // Initialize invariant data.
    numberDecimalDigits_ = 2;
    numberNegativePattern_ = 1;
    currencyDecimalDigits_ = 2;
    numberGroupSizes_ = arrayOf!(int)(3);
    currencyGroupSizes_ = arrayOf!(int)(3);
    numberGroupSeparator_ = ",";
    numberDecimalSeparator_ = ".";
    currencyGroupSeparator_ = ",";
    currencyDecimalSeparator_ = ".";
    currencySymbol_ = "\u00A4";
    negativeSign_ = "-";
    positiveSign_ = "+";
    nanSymbol_ = "NaN";
    negativeInfinitySymbol_ = "-Infinity";
    positiveInfinitySymbol_ = "Infinity";
    nativeDigits_ = arrayOf!(char[])("0", "1", "2", "3", "4", "5", "6", "7", "8", "9");

    if (cultureData !is null && cultureData.lcid != Culture.LCID_INVARIANT) {
      // Initialize culture-specific data.
      numberDecimalDigits_ = cultureData.digits;
      numberNegativePattern_ = cultureData.negativeNumber;
      currencyDecimalDigits_ = cultureData.currencyDigits;
      currencyNegativePattern_ = cultureData.negativeCurrency;
      currencyPositivePattern_ = cultureData.positiveCurrency;
      numberGroupSizes_ = cultureData.grouping;
      currencyGroupSizes_ = cultureData.monetaryGrouping;
      numberGroupSeparator_ = cultureData.thousand;
      numberDecimalSeparator_ = cultureData.decimal;
      currencyGroupSeparator_ = cultureData.monetaryThousand;
      currencyDecimalSeparator_ = cultureData.monetaryDecimal;
      currencySymbol_ = cultureData.currency;
      negativeSign_ = cultureData.negativeSign;
      positiveSign_ = cultureData.positiveSign;
      nanSymbol_ = cultureData.nan;
      negativeInfinitySymbol_ = cultureData.negInfinity;
      positiveInfinitySymbol_ = cultureData.posInfinity;
      nativeDigits_ = cultureData.nativeDigits;
    }
  }

  /**
   * Retrieves an object defining how to format the specified type.
   * Params: type = The TypeInfo of the resulting formatting object.
   * Returns: If type is typeid($(LINK2 #NumberFormat, NumberFormat)), the current NumberFormat instance. Otherwise, null.
   * Remarks: Implements $(LINK2 #IFormatService_getFormat, IFormatService.getFormat).
   */
  public Object getFormat(TypeInfo type) {
    return (type is typeid(NumberFormat)) ? this : null;
  }

  /**
   * Creates a copy of the instance.
   */
  public Object clone() {
    NumberFormat copy = cast(NumberFormat)cloneObject(this);
    copy.isReadOnly_ = false;
    return copy;
  }

  /**
   * Retrieves the NumberFormat for the specified $(LINK2 #IFormatService, IFormatService).
   * Params: formatService = The IFormatService used to retrieve NumberFormat.
   * Returns: The NumberFormat for the specified IFormatService.
   * Remarks: The method calls $(LINK2 #IFormatService_getFormat, IFormatService.getFormat) with typeof(NumberFormat). If formatService is null,
   * then the value of the current property is returned.
   */
  public static NumberFormat getInstance(IFormatService formatService) {
    Culture culture = cast(Culture)formatService;
    if (culture !is null) {
      if (culture.numberFormat_ !is null)
        return culture.numberFormat_;
      return culture.numberFormat;
    }
    if (NumberFormat numberFormat = cast(NumberFormat)formatService)
      return numberFormat;
    if (formatService !is null) {
      if (NumberFormat numberFormat = cast(NumberFormat)(formatService.getFormat(typeid(NumberFormat))))
        return numberFormat;
    }
    return current;
  }

  /**
   * $(I Property.) Retrieves a read-only NumberFormat instance from the current culture.
   * Returns: A read-only NumberFormat instance from the current culture.
   */
  public static NumberFormat current() {
    return Culture.current.numberFormat;
  }

  /**
   * $(ANCHOR NumberFormat_invariantFormat)
   * $(I Property.) Retrieves the read-only, culturally independent NumberFormat instance.
   * Returns: The read-only, culturally independent NumberFormat instance.
   */
  public static NumberFormat invariantFormat() {
    if (invariantFormat_ is null) {
      invariantFormat_ = new NumberFormat;
      invariantFormat_.isReadOnly_ = true;
    }
    return invariantFormat_;
  }

  /**
   * $(I Property.) Retrieves a value indicating whether the instance is read-only.
   * Returns: true if the instance is read-only; otherwise, false.
   */
  public final bool isReadOnly() {
    return isReadOnly_;
  }

  /**
   * $(I Property.) Retrieves the number of decimal places used for numbers.
   * Returns: The number of decimal places used for numbers. For $(LINK2 #NumberFormat_invariantFormat, invariantFormat), the default is 2.
   */
  public final int numberDecimalDigits() {
    return numberDecimalDigits_;
  }
  /**
   * Assigns the number of decimal digits used for numbers.
   * Params: value = The number of decimal places used for numbers.
   * Throws: Exception if the property is being set and the instance is read-only.
   * Examples:
   * The following example shows the effect of changing numberDecimalDigits.
   * ---
   * import tango.io.Print, tango.text.locale.Common;
   *
   * void main() {
   *   // Get the NumberFormat from the en-GB culture.
   *   NumberFormat fmt = (new Culture("en-GB")).numberFormat;
   *
   *   // Display a value with the default number of decimal digits.
   *   int n = 5678;
   *   Println(Formatter.format(fmt, "{0:N}", n));
   *
   *   // Display the value with six decimal digits.
   *   fmt.numberDecimalDigits = 6;
   *   Println(Formatter.format(fmt, "{0:N}", n));
   * }
   *
   * // Produces the following output:
   * // 5,678.00
   * // 5,678.000000
   * ---
   */
  public final void numberDecimalDigits(int value) {
    checkReadOnly();
    numberDecimalDigits_ = value;
  }

  /**
   * $(I Property.) Retrieves the format pattern for negative numbers.
   * Returns: The format pattern for negative numbers. For invariantFormat, the default is 1 (representing "-n").
   * Remarks: The following table shows valid values for this property.
   *
   * <table class="definitionTable">
   * <tr><th>Value</th><th>Pattern</th></tr>
   * <tr><td>0</td><td>(n)</td></tr>
   * <tr><td>1</td><td>-n</td></tr>
   * <tr><td>2</td><td>- n</td></tr>
   * <tr><td>3</td><td>n-</td></tr>
   * <tr><td>4</td><td>n -</td></tr>
   * </table>
   */
  public final int numberNegativePattern() {
    return numberNegativePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for negative numbers.
   * Params: value = The format pattern for negative numbers.
   * Examples:
   * The following example shows the effect of the different patterns.
   * ---
   * import tango.io.Print, tango.text.locale.Common;
   *
   * void main() {
   *   NumberFormat fmt = new NumberFormat;
   *   int n = -5678;
   *
   *   // Display the default pattern.
   *   Println(Formatter.format(fmt, "{0:N}", n));
   *
   *   // Display all patterns.
   *   for (int i = 0; i <= 4; i++) {
   *     fmt.numberNegativePattern = i;
   *     Println(Formatter.format(fmt, "{0:N}", n));
   *   }
   * }
   *
   * // Produces the following output:
   * // (5,678.00)
   * // (5,678.00)
   * // -5,678.00
   * // - 5,678.00
   * // 5,678.00-
   * // 5,678.00 -
   * ---
   */
  public final void numberNegativePattern(int value) {
    checkReadOnly();
    numberNegativePattern_ = value;
  }

  /**
   * $(I Property.) Retrieves the number of decimal places to use in currency values.
   * Returns: The number of decimal digits to use in currency values.
   */
  public final int currencyDecimalDigits() {
    return currencyDecimalDigits_;
  }
  /**
   * $(I Property.) Assigns the number of decimal places to use in currency values.
   * Params: value = The number of decimal digits to use in currency values.
   */
  public final void currencyDecimalDigits(int value) {
    checkReadOnly();
    currencyDecimalDigits_ = value;
  }

  /**
   * $(I Property.) Retrieves the formal pattern to use for negative currency values.
   * Returns: The format pattern to use for negative currency values.
   */
  public final int currencyNegativePattern() {
    return currencyNegativePattern_;
  }
  /**
   * $(I Property.) Assigns the formal pattern to use for negative currency values.
   * Params: value = The format pattern to use for negative currency values.
   */
  public final void currencyNegativePattern(int value) {
    checkReadOnly();
    currencyNegativePattern_ = value;
  }

  /**
   * $(I Property.) Retrieves the formal pattern to use for positive currency values.
   * Returns: The format pattern to use for positive currency values.
   */
  public final int currencyPositivePattern() {
    return currencyPositivePattern_;
  }
  /**
   * $(I Property.) Assigns the formal pattern to use for positive currency values.
   * Returns: The format pattern to use for positive currency values.
   */
  public final void currencyPositivePattern(int value) {
    checkReadOnly();
    currencyPositivePattern_ = value;
  }

  /**
   * $(I Property.) Retrieves the number of digits int each group to the left of the decimal place in numbers.
   * Returns: The number of digits int each group to the left of the decimal place in numbers.
   */
  public final int[] numberGroupSizes() {
    return numberGroupSizes_;
  }
  /**
   * $(I Property.) Assigns the number of digits int each group to the left of the decimal place in numbers.
   * Params: value = The number of digits int each group to the left of the decimal place in numbers.
   */
  public final void numberGroupSizes(int[] value) {
    checkReadOnly();
    numberGroupSizes_ = value;
  }

  /**
   * $(I Property.) Retrieves the number of digits int each group to the left of the decimal place in currency values.
   * Returns: The number of digits int each group to the left of the decimal place in currency values.
   */
  public final int[] currencyGroupSizes() {
    return currencyGroupSizes_;
  }
  /**
   * $(I Property.) Assigns the number of digits int each group to the left of the decimal place in currency values.
   * Params: value = The number of digits int each group to the left of the decimal place in currency values.
   */
  public final void currencyGroupSizes(int[] value) {
    checkReadOnly();
    currencyGroupSizes_ = value;
  }

  /**
   * $(I Property.) Retrieves the string separating groups of digits to the left of the decimal place in numbers.
   * Returns: The string separating groups of digits to the left of the decimal place in numbers. For example, ",".
   */
  public final char[] numberGroupSeparator() {
    return numberGroupSeparator_;
  }
  /**
   * $(I Property.) Assigns the string separating groups of digits to the left of the decimal place in numbers.
   * Params: value = The string separating groups of digits to the left of the decimal place in numbers.
   */
  public final void numberGroupSeparator(char[] value) {
    checkReadOnly();
    numberGroupSeparator_ = value;
  }

  /**
   * $(I Property.) Retrieves the string used as the decimal separator in numbers.
   * Returns: The string used as the decimal separator in numbers. For example, ".".
   */
  public final char[] numberDecimalSeparator() {
    return numberDecimalSeparator_;
  }
  /**
   * $(I Property.) Assigns the string used as the decimal separator in numbers.
   * Params: value = The string used as the decimal separator in numbers.
   */
  public final void numberDecimalSeparator(char[] value) {
    checkReadOnly();
    numberDecimalSeparator_ = value;
  }

  /**
   * $(I Property.) Retrieves the string separating groups of digits to the left of the decimal place in currency values.
   * Returns: The string separating groups of digits to the left of the decimal place in currency values. For example, ",".
   */
  public final char[] currencyGroupSeparator() {
    return currencyGroupSeparator_;
  }
  /**
   * $(I Property.) Assigns the string separating groups of digits to the left of the decimal place in currency values.
   * Params: value = The string separating groups of digits to the left of the decimal place in currency values.
   */
  public final void currencyGroupSeparator(char[] value) {
    checkReadOnly();
    currencyGroupSeparator_ = value;
  }

  /**
   * $(I Property.) Retrieves the string used as the decimal separator in currency values.
   * Returns: The string used as the decimal separator in currency values. For example, ".".
   */
  public final char[] currencyDecimalSeparator() {
    return currencyDecimalSeparator_;
  }
  /**
   * $(I Property.) Assigns the string used as the decimal separator in currency values.
   * Params: value = The string used as the decimal separator in currency values.
   */
  public final void currencyDecimalSeparator(char[] value) {
    checkReadOnly();
    currencyDecimalSeparator_ = value;
  }

  /**
   * $(I Property.) Retrieves the string used as the currency symbol.
   * Returns: The string used as the currency symbol. For example, "£".
   */
  public final char[] currencySymbol() {
    return currencySymbol_;
  }
  /**
   * $(I Property.) Assigns the string used as the currency symbol.
   * Params: value = The string used as the currency symbol.
   */
  public final void currencySymbol(char[] value) {
    checkReadOnly();
    currencySymbol_ = value;
  }

  /**
   * $(I Property.) Retrieves the string denoting that a number is negative.
   * Returns: The string denoting that a number is negative. For example, "-".
   */
  public final char[] negativeSign() {
    return negativeSign_;
  }
  /**
   * $(I Property.) Assigns the string denoting that a number is negative.
   * Params: value = The string denoting that a number is negative.
   */
  public final void negativeSign(char[] value) {
    checkReadOnly();
    negativeSign_ = value;
  }

  /**
   * $(I Property.) Retrieves the string denoting that a number is positive.
   * Returns: The string denoting that a number is positive. For example, "+".
   */
  public final char[] positiveSign() {
    return positiveSign_;
  }
  /**
   * $(I Property.) Assigns the string denoting that a number is positive.
   * Params: value = The string denoting that a number is positive.
   */
  public final void positiveSign(char[] value) {
    checkReadOnly();
    positiveSign_ = value;
  }

  /**
   * $(I Property.) Retrieves the string representing the NaN (not a number) value.
   * Returns: The string representing the NaN value. For example, "NaN".
   */
  public final char[] nanSymbol() {
    return nanSymbol_;
  }
  /**
   * $(I Property.) Assigns the string representing the NaN (not a number) value.
   * Params: value = The string representing the NaN value.
   */
  public final void nanSymbol(char[] value) {
    checkReadOnly();
    nanSymbol_ = value;
  }

  /**
   * $(I Property.) Retrieves the string representing negative infinity.
   * Returns: The string representing negative infinity. For example, "-Infinity".
   */
  public final char[] negativeInfinitySymbol() {
    return negativeInfinitySymbol_;
  }
  /**
   * $(I Property.) Assigns the string representing negative infinity.
   * Params: value = The string representing negative infinity.
   */
  public final void negativeInfinitySymbol(char[] value) {
    checkReadOnly();
    negativeInfinitySymbol_ = value;
  }

  /**
   * $(I Property.) Retrieves the string representing positive infinity.
   * Returns: The string representing positive infinity. For example, "Infinity".
   */
  public final char[] positiveInfinitySymbol() {
    return positiveInfinitySymbol_;
  }
  /**
   * $(I Property.) Assigns the string representing positive infinity.
   * Params: value = The string representing positive infinity.
   */
  public final void positiveInfinitySymbol(char[] value) {
    checkReadOnly();
    positiveInfinitySymbol_ = value;
  }

  /**
   * $(I Property.) Retrieves a string array of native equivalents of the digits 0 to 9.
   * Returns: A string array of native equivalents of the digits 0 to 9.
   */
  public final char[][] nativeDigits() {
    return nativeDigits_;
  }
  /**
   * $(I Property.) Assigns a string array of native equivalents of the digits 0 to 9.
   * Params: value = A string array of native equivalents of the digits 0 to 9.
   */
  public final void nativeDigits(char[][] value) {
    checkReadOnly();
    nativeDigits_ = value;
  }

  private void checkReadOnly() {
    if (isReadOnly_)
      throw new Exception("NumberFormat instance is read-only.");
  }

}

/**
 * $(ANCHOR _DateTimeFormat)
 * Determines how $(LINK2 #DateTime, DateTime) values are formatted, depending on the culture.
 * Remarks: To create a DateTimeFormat for a specific culture, create a $(LINK2 #Culture, Culture) for that culture and
 * retrieve its $(LINK2 #Culture_dateTimeFormat, dateTimeFormat) property. To create a DateTimeFormat for the user's current 
 * culture, use the $(LINK2 #Culture_current, current) property.
 */
public class DateTimeFormat : IFormatService {

  private const char[] rfc1123Pattern_ = "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'";
  private const char[] sortableDateTimePattern_ = "yyyy'-'MM'-'dd'T'HH':'mm':'ss";
  private const char[] universalSortableDateTimePattern_ = "yyyy'-'MM'-'dd' 'HH':'mm':'ss'Z'";

  package bool isReadOnly_;
  private static DateTimeFormat invariantFormat_;
  private CultureData* cultureData_;

  private Calendar calendar_;
  private int[] optionalCalendars_;
  private int firstDayOfWeek_ = -1;
  private int calendarWeekRule_ = -1;
  private char[] dateSeparator_;
  private char[] timeSeparator_;
  private char[] amDesignator_;
  private char[] pmDesignator_;
  private char[] shortDatePattern_;
  private char[] shortTimePattern_;
  private char[] longDatePattern_;
  private char[] longTimePattern_;
  private char[] monthDayPattern_;
  private char[] yearMonthPattern_;
  private char[][] abbreviatedDayNames_;
  private char[][] dayNames_;
  private char[][] abbreviatedMonthNames_;
  private char[][] monthNames_;

  private char[] fullDateTimePattern_;
  private char[] generalShortTimePattern_;
  private char[] generalLongTimePattern_;

  private char[][] shortTimePatterns_;
  private char[][] shortDatePatterns_;
  private char[][] longTimePatterns_;
  private char[][] longDatePatterns_;
  private char[][] yearMonthPatterns_;

  /**
   * $(ANCHOR DateTimeFormat_ctor)
   * Initializes an instance that is writable and culture-independent.
   */
  public this() {
    // This ctor is used by invariantFormat so we can't set the calendar property.
    cultureData_ = Culture.invariantCulture.cultureData_;
    calendar_ = GregorianCalendar.getDefaultInstance();
    initialize();
  }

  package this(CultureData* cultureData, Calendar calendar) {
    cultureData_ = cultureData;
    this.calendar = calendar;
  }

  /**
   * $(ANCHOR DateTimeFormat_getFormat)
   * Retrieves an object defining how to format the specified type.
   * Params: type = The TypeInfo of the resulting formatting object.
   * Returns: If type is typeid(DateTimeFormat), the current DateTimeFormat instance. Otherwise, null.
   * Remarks: Implements $(LINK2 #IFormatService_getFormat, IFormatService.getFormat).
   */
  public Object getFormat(TypeInfo type) {
    return (type is typeid(DateTimeFormat)) ? this : null;
  }

  /**
   */
  public Object clone() {
    DateTimeFormat other = cast(DateTimeFormat)cloneObject(this);
    other.calendar_ = cast(Calendar)calendar.clone();
    other.isReadOnly_ = false;
    return other;
  }

  package char[][] shortTimePatterns() {
    if (shortTimePatterns_ == null)
      shortTimePatterns_ = cultureData_.shortTimes;
    return shortTimePatterns_.dup;
  }

  package char[][] shortDatePatterns() {
    if (shortDatePatterns_ == null)
      shortDatePatterns_ = cultureData_.shortDates;
    return shortDatePatterns_.dup;
  }

  package char[][] longTimePatterns() {
    if (longTimePatterns_ == null)
      longTimePatterns_ = cultureData_.longTimes;
    return longTimePatterns_.dup;
  }

  package char[][] longDatePatterns() {
    if (longDatePatterns_ == null)
      longDatePatterns_ = cultureData_.longDates;
    return longDatePatterns_.dup;
  }

  package char[][] yearMonthPatterns() {
    if (yearMonthPatterns_ == null)
      yearMonthPatterns_ = cultureData_.yearMonths;
    return yearMonthPatterns_;
  }

  /**
   * $(ANCHOR DateTimeFormat_getAllDateTimePatterns)
   * Retrieves the standard patterns in which DateTime values can be formatted.
   * Returns: An array of strings containing the standard patterns in which DateTime values can be formatted.
   */
  public final char[][] getAllDateTimePatterns() {
    char[][] result;
    foreach (char format; allStandardFormats)
      result ~= getAllDateTimePatterns(format);
    return result;
  }

  /**
   * $(ANCHOR DateTimeFormat_getAllDateTimePatterns_char)
   * Retrieves the standard patterns in which DateTime values can be formatted using the specified format character.
   * Returns: An array of strings containing the standard patterns in which DateTime values can be formatted using the specified format character.
   */
  public final char[][] getAllDateTimePatterns(char format) {

    char[][] combinePatterns(char[][] patterns1, char[][] patterns2) {
      char[][] result = new char[][patterns1.length * patterns2.length];
      for (int i = 0; i < patterns1.length; i++) {
        for (int j = 0; j < patterns2.length; j++)
          result[i * patterns2.length + j] = patterns1[i] ~ " " ~ patterns2[j];
      }
      return result;
    }

    // format must be one of allStandardFormats.
    char[][] result;
    switch (format) {
      case 'd':
        result ~= shortDatePatterns;
        break;
      case 'D':
        result ~= longDatePatterns;
        break;
      case 'f':
        result ~= combinePatterns(longDatePatterns, shortTimePatterns);
        break;
      case 'F':
        result ~= combinePatterns(longDatePatterns, longTimePatterns);
        break;
      case 'g':
        result ~= combinePatterns(shortDatePatterns, shortTimePatterns);
        break;
      case 'G':
        result ~= combinePatterns(shortDatePatterns, longTimePatterns);
        break;
      case 'm':
      case 'M':
        result ~= monthDayPattern;
        break;
      case 'r':
      case 'R':
        result ~= rfc1123Pattern_;
        break;
      case 's':
        result ~= sortableDateTimePattern_;
        break;
      case 't':
        result ~= shortTimePatterns;
        break;
      case 'T':
        result ~= longTimePatterns;
      case 'u':
        result ~= universalSortableDateTimePattern_;
        break;
      case 'U':
        result ~= combinePatterns(longDatePatterns, longTimePatterns);
        break;
      case 'y':
      case 'Y':
        result ~= yearMonthPatterns;
        break;
      default:
        throw new Exception("The specified format was not valid.");
    }
    return result;
  }

  /**
   * $(ANCHOR DateTimeFormat_getAbbreviatedDayName)
   * Retrieves the abbreviated name of the specified day of the week based on the culture of the instance.
   * Params: dayOfWeek = A DayOfWeek value.
   * Returns: The abbreviated name of the day of the week represented by dayOfWeek.
   */
  public final char[] getAbbreviatedDayName(DayOfWeek dayOfWeek) {
    return abbreviatedDayNames[cast(int)dayOfWeek];
  }

  /**
   * $(ANCHOR DateTimeFormat_getDayName)
   * Retrieves the full name of the specified day of the week based on the culture of the instance.
   * Params: dayOfWeek = A DayOfWeek value.
   * Returns: The full name of the day of the week represented by dayOfWeek.
   */
  public final char[] getDayName(DayOfWeek dayOfWeek) {
    return dayNames[cast(int)dayOfWeek];
  }

  /**
   * $(ANCHOR DateTimeFormat_getAbbreviatedMonthName)
   * Retrieves the abbreviated name of the specified month based on the culture of the instance.
   * Params: month = An integer between 1 and 13 indicating the name of the _month to return.
   * Returns: The abbreviated name of the _month represented by month.
   */
  public final char[] getAbbreviatedMonthName(int month) {
    return abbreviatedMonthNames[month - 1];
  }

  /**
   * $(ANCHOR DateTimeFormat_getMonthName)
   * Retrieves the full name of the specified month based on the culture of the instance.
   * Params: month = An integer between 1 and 13 indicating the name of the _month to return.
   * Returns: The full name of the _month represented by month.
   */
  public final char[] getMonthName(int month) {
    return monthNames[month - 1];
  }

  /**
   * $(ANCHOR DateTimeFormat_getInstance)
   * Retrieves the DateTimeFormat for the specified IFormatService.
   * Params: formatService = The IFormatService used to retrieve DateTimeFormat.
   * Returns: The DateTimeFormat for the specified IFormatService.
   * Remarks: The method calls $(LINK2 #IFormatService_getFormat, IFormatService.getFormat) with typeof(DateTimeFormat). If formatService is null,
   * then the value of the current property is returned.
   */
  public static DateTimeFormat getInstance(IFormatService formatService) {
    Culture culture = cast(Culture)formatService;
    if (culture !is null) {
      if (culture.dateTimeFormat_ !is null)
        return culture.dateTimeFormat_;
      return culture.dateTimeFormat;
    }
    if (DateTimeFormat dateTimeFormat = cast(DateTimeFormat)formatService)
      return dateTimeFormat;
    if (formatService !is null) {
      if (DateTimeFormat dateTimeFormat = cast(DateTimeFormat)(formatService.getFormat(typeid(DateTimeFormat))))
        return dateTimeFormat;
    }
    return current;
  }

  /**
   * $(ANCHOR DateTimeFormat_current)
   * $(I Property.) Retrieves a read-only DateTimeFormat instance from the current culture.
   * Returns: A read-only DateTimeFormat instance from the current culture.
   */
  public static DateTimeFormat current() {
    return Culture.current.dateTimeFormat;
  }

  /**
   * $(ANCHOR DateTimeFormat_invariantFormat)
   * $(I Property.) Retrieves a read-only DateTimeFormat instance that is culturally independent.
   * Returns: A read-only DateTimeFormat instance that is culturally independent.
   */
  public static DateTimeFormat invariantFormat() {
    if (invariantFormat_ is null) {
      invariantFormat_ = new DateTimeFormat;
      invariantFormat_.calendar.isReadOnly_ = true;
      invariantFormat_.isReadOnly_ = true;
    }
    return invariantFormat_;
  }

  /**
   * $(ANCHOR DateTimeFormat_isReadOnly)
   * $(I Property.) Retrieves a value indicating whether the instance is read-only.
   * Returns: true is the instance is read-only; otherwise, false.
   */
  public final bool isReadOnly() {
    return isReadOnly_;
  }

  /**
   * $(I Property.) Retrieves the calendar used by the current culture.
   * Returns: The Calendar determining the calendar used by the current culture. For example, the GregorianCalendar.
   */
  public final Calendar calendar() {
    assert(calendar_ !is null);
    return calendar_;
  }
  /**
   * $(ANCHOR DateTimeFormat_calendar)
   * $(I Property.) Assigns the calendar to be used by the current culture.
   * Params: value = The Calendar determining the calendar to be used by the current culture.
   * Exceptions: If value is not valid for the current culture, an Exception is thrown.
   */
  public final void calendar(Calendar value) {
    checkReadOnly();
    if (value !is calendar_) {
      for (int i = 0; i < optionalCalendars.length; i++) {
        if (optionalCalendars[i] == value.id) {
          if (calendar_ !is null) {
            // Clear current properties.
            shortDatePattern_ = null;
            longDatePattern_ = null;
            shortTimePattern_ = null;
            yearMonthPattern_ = null;
            monthDayPattern_ = null;
            generalShortTimePattern_ = null;
            generalLongTimePattern_ = null;
            fullDateTimePattern_ = null;
            shortDatePatterns_ = null;
            longDatePatterns_ = null;
            yearMonthPatterns_ = null;
            abbreviatedDayNames_ = null;
            abbreviatedMonthNames_ = null;
            dayNames_ = null;
            monthNames_ = null;
          }
          calendar_ = value;
          initialize();
          return;
        }
      }
      throw new Exception("Not a valid calendar for the culture.");
    }
  }

  /**
   * $(ANCHOR DateTimeFormat_firstDayOfWeek)
   * $(I Property.) Retrieves the first day of the week.
   * Returns: A DayOfWeek value indicating the first day of the week.
   */
  public final DayOfWeek firstDayOfWeek() {
    return cast(DayOfWeek)firstDayOfWeek_;
  }
  /**
   * $(I Property.) Assigns the first day of the week.
   * Params: valie = A DayOfWeek value indicating the first day of the week.
   */
  public final void firstDayOfWeek(DayOfWeek value) {
    checkReadOnly();
    firstDayOfWeek_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_calendarWeekRule)
   * $(I Property.) Retrieves the _value indicating the rule used to determine the first week of the year.
   * Returns: A CalendarWeekRule _value determining the first week of the year.
   */
  public final CalendarWeekRule calendarWeekRule() {
    return cast(CalendarWeekRule)calendarWeekRule_;
  }
  /**
   * $(I Property.) Assigns the _value indicating the rule used to determine the first week of the year.
   * Params: value = A CalendarWeekRule _value determining the first week of the year.
   */
  public final void calendarWeekRule(CalendarWeekRule value) {
    checkReadOnly();
    calendarWeekRule_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_nativeCalendarName)
   * $(I Property.) Retrieves the native name of the calendar associated with the current instance.
   * Returns: The native name of the calendar associated with the current instance.
   */
  public final char[] nativeCalendarName() {
    return cultureData_.nativeCalName;
  }

  /**
   * $(ANCHOR DateTimeFormat_dateSeparator)
   * $(I Property.) Retrieves the string separating date components.
   * Returns: The string separating date components.
   */
  public final char[] dateSeparator() {
    if (dateSeparator_ == null)
      dateSeparator_ = cultureData_.date;
    return dateSeparator_;
  }
  /**
   * $(I Property.) Assigns the string separating date components.
   * Params: value = The string separating date components.
   */
  public final void dateSeparator(char[] value) {
    checkReadOnly();
    dateSeparator_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_timeSeparator)
   * $(I Property.) Retrieves the string separating time components.
   * Returns: The string separating time components.
   */
  public final char[] timeSeparator() {
    if (timeSeparator_ == null)
      timeSeparator_ = cultureData_.time;
    return timeSeparator_;
  }
  /**
   * $(I Property.) Assigns the string separating time components.
   * Params: value = The string separating time components.
   */
  public final void timeSeparator(char[] value) {
    checkReadOnly();
    timeSeparator_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_amDesignator)
   * $(I Property.) Retrieves the string designator for hours before noon.
   * Returns: The string designator for hours before noon. For example, "AM".
   */
  public final char[] amDesignator() {
    assert(amDesignator_ != null);
    return amDesignator_;
  }
  /**
   * $(I Property.) Assigns the string designator for hours before noon.
   * Params: value = The string designator for hours before noon.
   */
  public final void amDesignator(char[] value) {
    checkReadOnly();
    amDesignator_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_pmDesignator)
   * $(I Property.) Retrieves the string designator for hours after noon.
   * Returns: The string designator for hours after noon. For example, "PM".
   */
  public final char[] pmDesignator() {
    assert(pmDesignator_ != null);
    return pmDesignator_;
  }
  /**
   * $(I Property.) Assigns the string designator for hours after noon.
   * Params: value = The string designator for hours after noon.
   */
  public final void pmDesignator(char[] value) {
    checkReadOnly();
    pmDesignator_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_shortDatePattern)
   * $(I Property.) Retrieves the format pattern for a short date value.
   * Returns: The format pattern for a short date value.
   */
  public final char[] shortDatePattern() {
    assert(shortDatePattern_ != null);
    return shortDatePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a short date _value.
   * Params: value = The format pattern for a short date _value.
   */
  public final void shortDatePattern(char[] value) {
    checkReadOnly();
    if (shortDatePatterns_ != null)
      shortDatePatterns_[0] = value;
    shortDatePattern_ = value;
    generalLongTimePattern_ = null;
    generalShortTimePattern_ = null;
  }

  /**
   * $(ANCHOR DateTimeFormat_shortTimePattern)
   * $(I Property.) Retrieves the format pattern for a short time value.
   * Returns: The format pattern for a short time value.
   */
  public final char[] shortTimePattern() {
    if (shortTimePattern_ == null)
      shortTimePattern_ = cultureData_.shortTime;
    return shortTimePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a short time _value.
   * Params: value = The format pattern for a short time _value.
   */
  public final void shortTimePattern(char[] value) {
    checkReadOnly();
    shortTimePattern_ = value;
    generalShortTimePattern_ = null;
  }

  /**
   * $(ANCHOR DateTimeFormat_longDatePattern)
   * $(I Property.) Retrieves the format pattern for a long date value.
   * Returns: The format pattern for a long date value.
   */
  public final char[] longDatePattern() {
    assert(longDatePattern_ != null);
    return longDatePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a long date _value.
   * Params: value = The format pattern for a long date _value.
   */
  public final void longDatePattern(char[] value) {
    checkReadOnly();
    if (longDatePatterns_ != null)
      longDatePatterns_[0] = value;
    longDatePattern_ = value;
    fullDateTimePattern_ = null;
  }

  /**
   * $(ANCHOR DateTimeFormat_longTimePattern)
   * $(I Property.) Retrieves the format pattern for a long time value.
   * Returns: The format pattern for a long time value.
   */
  public final char[] longTimePattern() {
    assert(longTimePattern_ != null);
    return longTimePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a long time _value.
   * Params: value = The format pattern for a long time _value.
   */
  public final void longTimePattern(char[] value) {
    checkReadOnly();
    longTimePattern_ = value;
    fullDateTimePattern_ = null;
  }

  /**
   * $(ANCHOR DateTimeFormat_monthDayPattern)
   * $(I Property.) Retrieves the format pattern for a month and day value.
   * Returns: The format pattern for a month and day value.
   */
  public final char[] monthDayPattern() {
    if (monthDayPattern_ == null)
      monthDayPattern_ = cultureData_.monthDay;
    return monthDayPattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a month and day _value.
   * Params: value = The format pattern for a month and day _value.
   */
  public final void monthDayPattern(char[] value) {
    checkReadOnly();
    monthDayPattern_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_yearMonthPattern)
   * $(I Property.) Retrieves the format pattern for a year and month value.
   * Returns: The format pattern for a year and month value.
   */
  public final char[] yearMonthPattern() {
    assert(yearMonthPattern_ != null);
    return yearMonthPattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a year and month _value.
   * Params: value = The format pattern for a year and month _value.
   */
  public final void yearMonthPattern(char[] value) {
    checkReadOnly();
    yearMonthPattern_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_abbreviatedDayNames)
   * $(I Property.) Retrieves a string array containing the abbreviated names of the days of the week.
   * Returns: A string array containing the abbreviated names of the days of the week. For $(LINK2 #DateTimeFormat_invariantFormat, invariantFormat),
   *   this contains "Sun", "Mon", "Tue", "Wed", "Thu", "Fri" and "Sat".
   */
  public final char[][] abbreviatedDayNames() {
    if (abbreviatedDayNames_ == null)
      abbreviatedDayNames_ = cultureData_.abbrevDayNames;
    return abbreviatedDayNames_.dup;
  }
  /**
   * $(I Property.) Assigns a string array containing the abbreviated names of the days of the week.
   * Params: value = A string array containing the abbreviated names of the days of the week.
   */
  public final void abbreviatedDayNames(char[][] value) {
    checkReadOnly();
    abbreviatedDayNames_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_dayNames)
   * $(I Property.) Retrieves a string array containing the full names of the days of the week.
   * Returns: A string array containing the full names of the days of the week. For $(LINK2 #DateTimeFormat_invariantFormat, invariantFormat),
   *   this contains "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday" and "Saturday".
   */
  public final char[][] dayNames() {
    if (dayNames_ == null)
      dayNames_ = cultureData_.dayNames;
    return dayNames_.dup;
  }
  /**
   * $(I Property.) Assigns a string array containing the full names of the days of the week.
   * Params: value = A string array containing the full names of the days of the week.
   */
  public final void dayNames(char[][] value) {
    checkReadOnly();
    dayNames_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_abbreviatedMonthNames)
   * $(I Property.) Retrieves a string array containing the abbreviated names of the months.
   * Returns: A string array containing the abbreviated names of the months. For $(LINK2 #DateTimeFormat_invariantFormat, invariantFormat),
   *   this contains "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" and "".
   */
  public final char[][] abbreviatedMonthNames() {
    if (abbreviatedMonthNames_ == null)
      abbreviatedMonthNames_ = cultureData_.abbrevMonthNames;
    return abbreviatedMonthNames_.dup;
  }
  /**
   * $(I Property.) Assigns a string array containing the abbreviated names of the months.
   * Params: value = A string array containing the abbreviated names of the months.
   */
  public final void abbreviatedMonthNames(char[][] value) {
    checkReadOnly();
    abbreviatedMonthNames_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_monthNames)
   * $(I Property.) Retrieves a string array containing the full names of the months.
   * Returns: A string array containing the full names of the months. For $(LINK2 #DateTimeFormat_invariantFormat, invariantFormat),
   *   this contains "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" and "".
   */
  public final char[][] monthNames() {
    if (monthNames_ == null)
      monthNames_ = cultureData_.monthNames;
    return monthNames_.dup;
  }
  /**
   * $(I Property.) Assigns a string array containing the full names of the months.
   * Params: value = A string array containing the full names of the months.
   */
  public final void monthNames(char[][] value) {
    checkReadOnly();
    monthNames_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_fullDateTimePattern)
   * $(I Property.) Retrieves the format pattern for a long date and a long time value.
   * Returns: The format pattern for a long date and a long time value.
   */
  public final char[] fullDateTimePattern() {
    if (fullDateTimePattern_ == null)
      fullDateTimePattern_ = longDatePattern ~ " " ~ longTimePattern;
    return fullDateTimePattern_;
  }
  /**
   * $(I Property.) Assigns the format pattern for a long date and a long time _value.
   * Params: value = The format pattern for a long date and a long time _value.
   */
  public final void fullDateTimePattern(char[] value) {
    checkReadOnly();
    fullDateTimePattern_ = value;
  }

  /**
   * $(ANCHOR DateTimeFormat_rfc1123Pattern)
   * $(I Property.) Retrieves the format pattern based on the IETF RFC 1123 specification, for a time value.
   * Returns: The format pattern based on the IETF RFC 1123 specification, for a time value.
   */
  public final char[] rfc1123Pattern() {
    return rfc1123Pattern_;
  }

  /**
   * $(ANCHOR DateTimeFormat_sortableDateTimePattern)
   * $(I Property.) Retrieves the format pattern for a sortable date and time value.
   * Returns: The format pattern for a sortable date and time value.
   */
  public final char[] sortableDateTimePattern() {
    return sortableDateTimePattern_;
  }

  /**
   * $(ANCHOR DateTimeFormat_universalSortableDateTimePattern)
   * $(I Property.) Retrieves the format pattern for a universal date and time value.
   * Returns: The format pattern for a universal date and time value.
   */
  public final char[] universalSortableDateTimePattern() {
    return universalSortableDateTimePattern_;
  }

  package char[] generalShortTimePattern() {
    if (generalShortTimePattern_ == null)
      generalShortTimePattern_ = shortDatePattern ~ " " ~ shortTimePattern;
    return generalShortTimePattern_;
  }

  package char[] generalLongTimePattern() {
    if (generalLongTimePattern_ == null)
      generalLongTimePattern_ = shortDatePattern ~ " " ~ longTimePattern;
    return generalLongTimePattern_;
  }

  private void checkReadOnly() {
    if (isReadOnly_)
      throw new Exception("DateTimeFormat instance is read-only.");
  }

  private void initialize() {
    if (longTimePattern_ == null)
      longTimePattern_ = cultureData_.longTime;
    if (shortDatePattern_ == null)
      shortDatePattern_ = cultureData_.shortDate;
    if (longDatePattern_ == null)
      longDatePattern_ = cultureData_.longDate;
    if (yearMonthPattern_ == null)
      yearMonthPattern_ = cultureData_.yearMonth;
    if (amDesignator_ == null)
      amDesignator_ = cultureData_.am;
    if (pmDesignator_ == null)
      pmDesignator_ = cultureData_.pm;
    if (firstDayOfWeek_ == -1)
      firstDayOfWeek_ = cultureData_.firstDayOfWeek;
    if (calendarWeekRule_ == -1)
      calendarWeekRule_ = cultureData_.firstDayOfYear;
  }

  private int[] optionalCalendars() {
    if (optionalCalendars_ is null)
      optionalCalendars_ = cultureData_.optionalCalendars;
    return optionalCalendars_;
  }

}

package struct EraRange {

  private static EraRange[][int] eraRanges;
  private static int[int] currentEras;
  private static bool initialized_;

  package int era;
  package ulong ticks;
  package int yearOffset;
  package int minEraYear;
  package int maxEraYear;

  private static void initialize() {
    if (!initialized_) {
      eraRanges[Calendar.JAPAN] ~= EraRange(4, DateTime(1989, 1, 8).ticks, 1988, 1, GregorianCalendar.MAX_YEAR);
      eraRanges[Calendar.JAPAN] ~= EraRange(3, DateTime(1926, 12, 25).ticks, 1925, 1, 1989);
      eraRanges[Calendar.JAPAN] ~= EraRange(2, DateTime(1912, 7, 30).ticks, 1911, 1, 1926);
      eraRanges[Calendar.JAPAN] ~= EraRange(1, DateTime(1868, 9, 8).ticks, 1867, 1, 1912);
      eraRanges[Calendar.TAIWAN] ~= EraRange(1, DateTime(1912, 1, 1).ticks, 1911, 1, GregorianCalendar.MAX_YEAR);
      eraRanges[Calendar.KOREA] ~= EraRange(1, DateTime(1, 1, 1).ticks, -2333, 2334, GregorianCalendar.MAX_YEAR);
      eraRanges[Calendar.THAI] ~= EraRange(1, DateTime(1, 1, 1).ticks, -543, 544, GregorianCalendar.MAX_YEAR);
      currentEras[Calendar.JAPAN] = 4;
      currentEras[Calendar.TAIWAN] = 1;
      currentEras[Calendar.KOREA] = 1;
      currentEras[Calendar.THAI] = 1;
      initialized_ = true;
    }
  }

  package static EraRange[] getEraRanges(int calID) {
    if (!initialized_)
      initialize();
    return eraRanges[calID];
  }

  package static int getCurrentEra(int calID) {
    if (!initialized_)
      initialize();
    return currentEras[calID];
  }

  private static EraRange opCall(int era, ulong ticks, int yearOffset, int minEraYear, int prevEraYear) {
    EraRange eraRange;
    eraRange.era = era;
    eraRange.ticks = ticks;
    eraRange.yearOffset = yearOffset;
    eraRange.minEraYear = minEraYear;
    eraRange.maxEraYear = prevEraYear - yearOffset;
    return eraRange;
  }

}

// Calendars
// Apart from GreogrianCalendar, these are pretty much untested.

/**
 * $(ANCHOR _Calendar)
 * Represents time in week, month and year divisions.
 * Remarks: Calendar is the abstract base class for the following Calendar implementations: 
 *   $(LINK2 #GregorianCalendar, GregorianCalendar), $(LINK2 #HebrewCalendar, HebrewCalendar), $(LINK2 #HijriCalendar, HijriCalendar),
 *   $(LINK2 #JapaneseCalendar, JapaneseCalendar), $(LINK2 #KoreanCalendar, KoreanCalendar), $(LINK2 #TaiwanCalendar, TaiwanCalendar) and
 *   $(LINK2 #ThaiBuddhistCalendar, ThaiBuddhistCalendar).
 */
public abstract class Calendar {

  /**
   * Indicates the current era of the calendar.
   */
  public const int CURRENT_ERA = 0;

  private int currentEra_ = -1;
  package bool isReadOnly_;

  /**
   * Creates a copy of the current instance.
   * Returns: A new Object that is a copy of the current instance.
   */
  public Object clone() {
    Calendar other = cast(Calendar)cloneObject(this);
    other.isReadOnly_ = false;
    return other;
  }

  /**
   * Returns a DateTime value set to the specified date and time in the current era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   * Returns: The DateTime set to the specified date and time.
   */
  public DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    return getDateTime(year, month, day, hour, minute, second, millisecond, CURRENT_ERA);
  }

  /**
   * When overridden, returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public abstract DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era);

  /**
   * When overridden, returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public abstract DayOfWeek getDayOfWeek(DateTime time);

  /**
   * When overridden, returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public abstract int getDayOfMonth(DateTime time);

  /**
   * When overridden, returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public abstract int getDayOfYear(DateTime time);

  /**
   * When overridden, returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public abstract int getMonth(DateTime time);

  /**
   * When overridden, returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public abstract int getYear(DateTime time);

  /**
   * When overridden, returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public abstract int getEra(DateTime time);

  /**
   * Returns the number of days in the specified _year and _month of the current era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   * Returns: The number of days in the specified _year and _month of the current era.
   */
  public int getDaysInMonth(int year, int month) {
    return getDaysInMonth(year, month, CURRENT_ERA);
  }

  /**
   * When overridden, returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public abstract int getDaysInMonth(int year, int month, int era);

  /**
   * Returns the number of days in the specified _year of the current era.
   * Params: year = An integer representing the _year.
   * Returns: The number of days in the specified _year in the current era.
   */
  public int getDaysInYear(int year) {
    return getDaysInYear(year, CURRENT_ERA);
  }

  /**
   * When overridden, returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public abstract int getDaysInYear(int year, int era);

  /**
   * Returns the number of months in the specified _year of the current era.
   * Params: year = An integer representing the _year.
   * Returns: The number of months in the specified _year in the current era.
   */
  public int getMonthsInYear(int year) {
    return getMonthsInYear(year, CURRENT_ERA);
  }

  /**
   * When overridden, returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public abstract int getMonthsInYear(int year, int era);

  /**
   * Returns the week of the year that includes the specified DateTime.
   * Params:
   *   time = A DateTime value.
   *   rule = A CalendarWeekRule value defining a calendar week.
   *   firstDayOfWeek = A DayOfWeek value representing the first day of the week.
   * Returns: An integer representing the week of the year that includes the date in time.
   */
  public int getWeekOfYear(DateTime time, CalendarWeekRule rule, DayOfWeek firstDayOfWeek) {
    int year = getYear(time);
    int jan1 = cast(int)getDayOfWeek(getDateTime(year, 1, 1, 0, 0, 0, 0));

    switch (rule) {
      case CalendarWeekRule.FirstDay:
        int n = jan1 - cast(int)firstDayOfWeek;
        if (n < 0)
          n += 7;
        return (getDayOfYear(time) + n - 1) / 7 + 1;
      case CalendarWeekRule.FirstFullWeek:
      case CalendarWeekRule.FirstFourDayWeek:
        int fullDays = (rule == CalendarWeekRule.FirstFullWeek) ? 7 : 4;
        int n = cast(int)firstDayOfWeek - jan1;
        if (n != 0) {
          if (n < 0)
            n += 7;
          else if (n >= fullDays)
            n -= 7;
        }
        int day = getDayOfYear(time) - n;
        if (day > 0)
          return (day - 1) / 7 + 1;
        year = getYear(time) - 1;
        int month = getMonthsInYear(year);
        day = getDaysInMonth(year, month);
        return getWeekOfYear(getDateTime(year, month, day, 0, 0, 0, 0), rule, firstDayOfWeek);
      default:
        break;
    }
    // To satisfy -w
    throw new Exception("Value was out of range.");
  }

  /**
   * Indicates whether the specified _year in the current era is a leap _year.
   * Params: year = An integer representing the _year.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public bool isLeapYear(int year) {
    return isLeapYear(year, CURRENT_ERA);
  }

  /**
   * When overridden, indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public abstract bool isLeapYear(int year, int era);

  /**
   * $(I Property.) When overridden, retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public abstract int[] eras();

  /**
   * $(I Property.) Retreives a value indicating whether the instance is read-only.
   * Returns: true if the instance is read-only; otherwise, false.
   */
  public final bool isReadOnly() {
    return isReadOnly_;
  }

  /**
   * $(I Property.) Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public int id() {
    return -1;
  }

  protected int currentEra() {
    if (currentEra_ == -1)
      currentEra_ = EraRange.getCurrentEra(id);
    return currentEra_;
  }

  // Corresponds to Win32 calendar IDs
  package enum {
    GREGORIAN = 1,
    GREGORIAN_US = 2,
    JAPAN = 3,
    TAIWAN = 4,
    KOREA = 5,
    HIJRI = 6,
    THAI = 7,
    HEBREW = 8,
    GREGORIAN_ME_FRENCH = 9,
    GREGORIAN_ARABIC = 10,
    GREGORIAN_XLIT_ENGLISH = 11,
    GREGORIAN_XLIT_FRENCH = 12
  }

}

/**
 * $(ANCHOR _GregorianCalendar)
 * Represents the Gregorian calendar.
*/
public class GregorianCalendar : Calendar {

  /**
   * Represents the current era.
   */
  public const int AD_ERA = 1;

  private const int MAX_YEAR = 9999;

  private static Calendar defaultInstance_;
  private GregorianCalendarTypes type_;

  /**
   * Initializes an instance of the GregorianCalendar class using the specified GregorianCalendarTypes value. If no value is 
   * specified, the default is GregorianCalendarTypes.Localized.
   */
  public this(GregorianCalendarTypes type = GregorianCalendarTypes.Localized) {
    type_ = type;
  }

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return DateTime(year, month, day, hour, minute, second, millisecond);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cast(DayOfWeek)((time.ticks / TICKS_PER_DAY + 1) % 7);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY_OF_YEAR);
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.MONTH);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return extractPart(time.ticks, DatePart.YEAR);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return AD_ERA;
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    int[] monthDays = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? DAYS_TO_MONTH_LEAP : DAYS_TO_MONTH_COMMON;
    return monthDays[month] - monthDays[month - 1];
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return isLeapYear(year, era) ? 366 : 365;
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return 12;
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  }

  /**
   * $(I Property.) Retrieves the GregorianCalendarTypes value indicating the language version of the GregorianCalendar.
   * Returns: The GregorianCalendarTypes value indicating the language version of the GregorianCalendar.
   */
  public GregorianCalendarTypes calendarType() {
    return type_;
  }
  /**
   * $(I Property.) Assigns the GregorianCalendarTypes _value indicating the language version of the GregorianCalendar.
   * Params: value = The GregorianCalendarTypes _value indicating the language version of the GregorianCalendar.
   */
  public void calendarType(GregorianCalendarTypes value) {
    checkReadOnly();
    type_ = value;
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return arrayOf!(int)(AD_ERA);
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return cast(int)type_;
  }

  package static Calendar getDefaultInstance() {
    if (defaultInstance_ is null)
      defaultInstance_ = new GregorianCalendar;
    return defaultInstance_;
  }

  private void checkReadOnly() {
    if (isReadOnly_)
      throw new Exception("Calendar instance is read-only.");
  }

}

private class GregorianBasedCalendar : Calendar {

  private EraRange[] eraRanges_;
  private int maxYear_, minYear_;

  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    year = getGregorianYear(year, era);
    return DateTime(year, month, day, hour, minute, second, millisecond);
  }

  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cast(DayOfWeek)((time.ticks / TICKS_PER_DAY + 1) % 7);
  }

  public override int getDayOfMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY);
  }

  public override int getDayOfYear(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY_OF_YEAR);
  }

  public override int getMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.MONTH);
  }

  public override int getYear(DateTime time) {
    ulong ticks = time.ticks;
    int year = extractPart(time.ticks, DatePart.YEAR);
    foreach (EraRange eraRange; eraRanges_) {
      if (ticks >= eraRange.ticks)
        return year - eraRange.yearOffset;
    }
    throw new Exception("Value was out of range.");
  }

  public override int getEra(DateTime time) {
    ulong ticks = time.ticks;
    foreach (EraRange eraRange; eraRanges_) {
      if (ticks >= eraRange.ticks)
        return eraRange.era;
    }
    throw new Exception("Value was out of range.");
  }

  public override int getDaysInMonth(int year, int month, int era) {
    int[] monthDays = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? DAYS_TO_MONTH_LEAP : DAYS_TO_MONTH_COMMON;
    return monthDays[month] - monthDays[month - 1];
  }

  public override int getDaysInYear(int year, int era) {
    return isLeapYear(year, era) ? 366 : 365;
  }

  public override int getMonthsInYear(int year, int era) {
    return 12;
  }

  public override bool isLeapYear(int year, int era) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  }

  public override int[] eras() {
    int[] result;
    foreach (EraRange eraRange; eraRanges_)
      result ~= eraRange.era;
    return result;
  }

  protected this() {
    eraRanges_ = EraRange.getEraRanges(id);
    maxYear_ = eraRanges_[0].maxEraYear;
    minYear_ = eraRanges_[0].minEraYear;
  }

  private int getGregorianYear(int year, int era) {
    if (era == 0)
      era = currentEra;
    foreach (EraRange eraRange; eraRanges_) {
      if (era == eraRange.era) {
        if (year >= eraRange.minEraYear && year <= eraRange.maxEraYear)
          return eraRange.yearOffset + year;
        throw new Exception("Value was out of range.");
      }
    }
    throw new Exception("Era value was not valid.");
  }

}

/**
 * $(ANCHOR _JapaneseCalendar)
 * Represents the Japanese calendar.
 */
public class JapaneseCalendar : Calendar {

  private GregorianBasedCalendar cal_;

  /**
   * Initializes an instance of the JapaneseCalendar class.
   */
  public this() {
    cal_ = new GregorianBasedCalendar;
  }

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return cal_.getDateTime(year, month, day, hour, minute, second, millisecond, era);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cal_.getDayOfWeek(time);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return cal_.getDayOfMonth(time);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return cal_.getDayOfYear(time);
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    return cal_.getMonth(time);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return cal_.getYear(time);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return cal_.getEra(time);
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    return cal_.getDaysInMonth(year, month, era);
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return cal_.getDaysInYear(year, era);
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return cal_.getMonthsInYear(year, era);
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return cal_.isLeapYear(year, era);
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return cal_.eras;
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.JAPAN;
  }

}

/**
 * $(ANCHOR _TaiwanCalendar)
 * Represents the Taiwan calendar.
 */
public class TaiwanCalendar : Calendar {

  private GregorianBasedCalendar cal_;

  /**
   * Initializes a new instance of the TaiwanCalendar class.
   */
  public this() {
    cal_ = new GregorianBasedCalendar;
  }

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return cal_.getDateTime(year, month, day, hour, minute, second, millisecond, era);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cal_.getDayOfWeek(time);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return cal_.getDayOfMonth(time);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return cal_.getDayOfYear(time);
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    return cal_.getMonth(time);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return cal_.getYear(time);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return cal_.getEra(time);
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    return cal_.getDaysInMonth(year, month, era);
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return cal_.getDaysInYear(year, era);
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return cal_.getMonthsInYear(year, era);
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return cal_.isLeapYear(year, era);
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return cal_.eras;
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.TAIWAN;
  }

}

/** 
 * $(ANCHOR _KoreanCalendar)
 * Represents the Korean calendar.
 */
public class KoreanCalendar : Calendar {

  private GregorianBasedCalendar cal_;

  /**
   * Initializes a new instance of the KoreanCalendar class.
   */
  public this() {
    cal_ = new GregorianBasedCalendar;
  }

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return cal_.getDateTime(year, month, day, hour, minute, second, millisecond, era);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cal_.getDayOfWeek(time);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return cal_.getDayOfMonth(time);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return cal_.getDayOfYear(time);
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    return cal_.getMonth(time);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return cal_.getYear(time);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return cal_.getEra(time);
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    return cal_.getDaysInMonth(year, month, era);
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return cal_.getDaysInYear(year, era);
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return cal_.getMonthsInYear(year, era);
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return cal_.isLeapYear(year, era);
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return cal_.eras;
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.KOREA;
  }

}

/**
 * $(ANCHOR _HijriCalendar)
 * Represents the Hijri calendar.
 */
public class HijriCalendar : Calendar {

  private static const int[] DAYS_TO_MONTH = [ 0, 30, 59, 89, 118, 148, 177, 207, 236, 266, 295, 325, 355 ];

  /**
   * Represents the current era.
   */
  public const int HIJRI_ERA = 1;

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return DateTime((daysSinceJan1(year, month, day) - 1) * TICKS_PER_DAY + TimeSpan.getTicks(hour, minute, second) + (millisecond * TICKS_PER_MILLISECOND));
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cast(DayOfWeek)(cast(int)(time.ticks / TICKS_PER_DAY + 1) % 7);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return extractPart(time.ticks, DatePart.DAY_OF_YEAR);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getMonth(DateTime time) {
    return extractPart(time.ticks, DatePart.MONTH);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return extractPart(time.ticks, DatePart.YEAR);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return HIJRI_ERA;
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    if (month == 12)
      return isLeapYear(year, CURRENT_ERA) ? 30 : 29;
    return (month % 2 == 1) ? 30 : 29;
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return isLeapYear(year, era) ? 355 : 354;
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return 12;
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return (14 + 11 * year) % 30 < 11;
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return arrayOf!(int)(HIJRI_ERA);
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.HIJRI;
  }

  private long daysToYear(int year) {
    int cycle = ((year - 1) / 30) * 30;
    int remaining = year - cycle - 1;
    long days = ((cycle * 10631L) / 30L) + 227013L;
    while (remaining > 0) {
      days += 354 + (isLeapYear(remaining, CURRENT_ERA) ? 1 : 0);
      remaining--;
    }
    return days;
  }

  private long daysSinceJan1(int year, int month, int day) {
    return cast(long)(daysToYear(year) + DAYS_TO_MONTH[month - 1] + day);
  }

  private int extractPart(ulong ticks, DatePart part) {
    long days = cast(long)(ticks / TICKS_PER_DAY + 1);
    int year = cast(int)(((days - 227013) * 30) / 10631) + 1;
    long daysUpToYear = daysToYear(year);
    long daysInYear = getDaysInYear(year, CURRENT_ERA);
    if (days < daysUpToYear) {
      daysUpToYear -= daysInYear;
      year--;
    }
    else if (days == daysUpToYear) {
      year--;
      daysUpToYear -= getDaysInYear(year, CURRENT_ERA);
    }
    else if (days > daysUpToYear + daysInYear) {
      daysUpToYear += daysInYear;
      year++;
    }

    if (part == DatePart.YEAR)
      return year;

    days -= daysUpToYear;
    if (part == DatePart.DAY_OF_YEAR)
      return cast(int)days;

    int month = 1;
    while (month <= 12 && days > DAYS_TO_MONTH[month - 1])
      month++;
    month--;
    if (part == DatePart.MONTH)
      return month;

    return cast(int)(days - DAYS_TO_MONTH[month - 1]);
  }

}

/**
 * $(ANCHOR _ThaiBuddhistCalendar)
 * Represents the Thai Buddhist calendar.
 */
public class ThaiBuddhistCalendar : Calendar {

  private GregorianBasedCalendar cal_;

  /**
   * Initializes a new instance of the ThaiBuddhistCalendar class.
   */
  public this() {
    cal_ = new GregorianBasedCalendar;
  }

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    return cal_.getDateTime(year, month, day, hour, minute, second, millisecond, era);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cal_.getDayOfWeek(time);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    return cal_.getDayOfMonth(time);
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    return cal_.getDayOfYear(time);
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    return cal_.getMonth(time);
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    return cal_.getYear(time);
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return cal_.getEra(time);
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    return cal_.getDaysInMonth(year, month, era);
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return cal_.getDaysInYear(year, era);
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return cal_.getMonthsInYear(year, era);
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    return cal_.isLeapYear(year, era);
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return cal_.eras;
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.THAI;
  }

}

/**
 * $(ANCHOR _HebrewCalendar)
 * Represents the Hebrew calendar.
 */
public class HebrewCalendar : Calendar {

  private const int[14][7] MONTHDAYS = [
    // month                                                    // year type
    [ 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 ], 
    [ 0, 30, 29, 29, 29, 30, 29, 30, 29, 30, 29, 30, 29, 0 ],   // 1
    [ 0, 30, 29, 30, 29, 30, 29, 30, 29, 30, 29, 30, 29, 0 ],   // 2
    [ 0, 30, 30, 30, 29, 30, 29, 30, 29, 30, 29, 30, 29, 0 ],   // 3
    [ 0, 30, 29, 29, 29, 30, 30, 29, 30, 29, 30, 29, 30, 29 ],  // 4
    [ 0, 30, 29, 30, 29, 30, 30, 29, 30, 29, 30, 29, 30, 29 ],  // 5
    [ 0, 30, 30, 30, 29, 30, 30, 29, 30, 29, 30, 29, 30, 29 ]   // 6
  ];

  private const int YEAROF1AD = 3760;
  private const int DAYS_TO_1AD = cast(int)(YEAROF1AD * 365.2735);

  private const int PARTS_PER_HOUR = 1080;
  private const int PARTS_PER_DAY = 24 * PARTS_PER_HOUR;
  private const int DAYS_PER_MONTH = 29;
  private const int DAYS_PER_MONTH_FRACTION = 12 * PARTS_PER_HOUR + 793;
  private const int PARTS_PER_MONTH = DAYS_PER_MONTH * PARTS_PER_DAY + DAYS_PER_MONTH_FRACTION;
  private const int FIRST_NEW_MOON = 11 * PARTS_PER_HOUR + 204;

  private int minYear_ = YEAROF1AD + 1583;
  private int maxYear_ = YEAROF1AD + 2240;

  /**
   * Represents the current era.
   */
  public const int HEBREW_ERA = 1;

  /**
   * Overridden. Returns a DateTime value set to the specified date and time in the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   day = An integer representing the _day.
   *   hour = An integer representing the _hour.
   *   minute = An integer representing the _minute.
   *   second = An integer representing the _second.
   *   millisecond = An integer representing the _millisecond.
   *   era = An integer representing the _era.
   * Returns: A DateTime set to the specified date and time.
   */
  public override DateTime getDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
    checkYear(year, era);
    return getGregorianDateTime(year, month, day, hour, minute, second, millisecond);
  }

  /**
   * Overridden. Returns the day of the week in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: A DayOfWeek value representing the day of the week of time.
   */
  public override DayOfWeek getDayOfWeek(DateTime time) {
    return cast(DayOfWeek)cast(int)((time.ticks / TICKS_PER_DAY + 1) % 7);
  }

  /**
   * Overridden. Returns the day of the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the month of time.
   */
  public override int getDayOfMonth(DateTime time) {
    int year = getYear(time);
    int yearType = getYearType(year);
    int days = getStartOfYear(year) - DAYS_TO_1AD;
    int day = cast(int)(time.ticks / TICKS_PER_DAY) - days;
    int n;
    while (n < 12 && day >= MONTHDAYS[yearType][n + 1]) {
      day -= MONTHDAYS[yearType][n + 1];
      n++;
    }
    return day + 1;
  }

  /**
   * Overridden. Returns the day of the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the day of the year of time.
   */
  public override int getDayOfYear(DateTime time) {
    int year = getYear(time);
    int days = getStartOfYear(year) - DAYS_TO_1AD;
    return (cast(int)(time.ticks / TICKS_PER_DAY) - days) + 1;
  }

  /**
   * Overridden. Returns the month in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the month in time.
   */
  public override int getMonth(DateTime time) {
    int year = getYear(time);
    int yearType = getYearType(year);
    int days = getStartOfYear(year) - DAYS_TO_1AD;
    int day = cast(int)(time.ticks / TICKS_PER_DAY) - days;
    int n;
    while (n < 12 && day >= MONTHDAYS[yearType][n + 1]) {
      day -= MONTHDAYS[yearType][n + 1];
      n++;
    }
    return n + 1;
  }

  /**
   * Overridden. Returns the year in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the year in time.
   */
  public override int getYear(DateTime time) {
    int day = cast(int)(time.ticks / TICKS_PER_DAY) + DAYS_TO_1AD;
    int low = minYear_, high = maxYear_;
    // Perform a binary search.
    while (low <= high) {
      int mid = low + (high - low) / 2;
      int startDay = getStartOfYear(mid);
      if (day < startDay)
        high = mid - 1;
      else if (day >= startDay && day < getStartOfYear(mid + 1))
        return mid;
      else
        low = mid + 1;
    }
    return low;
  }

  /**
   * Overridden. Returns the era in the specified DateTime.
   * Params: time = A DateTime value.
   * Returns: An integer representing the ear in time.
   */
  public override int getEra(DateTime time) {
    return HEBREW_ERA;
  }

  /**
   * Overridden. Returns the number of days in the specified _year and _month of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   month = An integer representing the _month.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year and _month of the specified _era.
   */
  public override int getDaysInMonth(int year, int month, int era) {
    checkYear(year, era);
    return MONTHDAYS[getYearType(year)][month];
  }

  /**
   * Overridden. Returns the number of days in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of days in the specified _year in the specified _era.
   */
  public override int getDaysInYear(int year, int era) {
    return getStartOfYear(year + 1) - getStartOfYear(year);
  }

  /**
   * Overridden. Returns the number of months in the specified _year of the specified _era.
   * Params:
   *   year = An integer representing the _year.
   *   era = An integer representing the _era.
   * Returns: The number of months in the specified _year in the specified _era.
   */
  public override int getMonthsInYear(int year, int era) {
    return isLeapYear(year, era) ? 13 : 12;
  }

  /**
   * Overridden. Indicates whether the specified _year in the specified _era is a leap _year.
   * Params: year = An integer representing the _year.
   * Params: era = An integer representing the _era.
   * Returns: true is the specified _year is a leap _year; otherwise, false.
   */
  public override bool isLeapYear(int year, int era) {
    checkYear(year, era);
    // true if year % 19 == 0, 3, 6, 8, 11, 14, 17
    return ((7 * year + 1) % 19) < 7;
  }

  /**
   * $(I Property.) Overridden. Retrieves the list of eras in the current calendar.
   * Returns: An integer array representing the eras in the current calendar.
   */
  public override int[] eras() {
    return arrayOf!(int)(HEBREW_ERA);
  }

  /**
   * $(I Property.) Overridden. Retrieves the identifier associated with the current calendar.
   * Returns: An integer representing the identifier of the current calendar.
   */
  public override int id() {
    return Calendar.HEBREW;
  }

  private void checkYear(int year, int era) {
    if ((era != CURRENT_ERA && era != HEBREW_ERA) || (year > maxYear_ || year < minYear_))
      throw new Exception("Value was out of range.");
  }

  private int getYearType(int year) {
    int yearLength = getStartOfYear(year + 1) - getStartOfYear(year);
    if (yearLength > 380)
      yearLength -= 30;
    switch (yearLength) {
      case 353:
        // "deficient"
        return 0;
      case 383:
        // "deficient" leap
        return 4;
      case 354:
        // "normal"
        return 1;
      case 384:
        // "normal" leap
        return 5;
      case 355:
        // "complete"
        return 2;
      case 385:
        // "complete" leap
        return 6;
      default:
        break;
    }
    // Satisfies -w
    throw new Exception("Value was not valid.");
  }

  private int getStartOfYear(int year) {
    int months = (235 * year - 234) / 19;
    int fraction = months * DAYS_PER_MONTH_FRACTION + FIRST_NEW_MOON;
    int day = months * 29 + (fraction / PARTS_PER_DAY);
    fraction %= PARTS_PER_DAY;

    int dayOfWeek = day % 7;
    if (dayOfWeek == 2 || dayOfWeek == 4 || dayOfWeek == 6) {
      day++;
      dayOfWeek = day % 7;
    }
    if (dayOfWeek == 1 && fraction > 15 * PARTS_PER_HOUR + 204 && !isLeapYear(year, CURRENT_ERA))
      day += 2;
    else if (dayOfWeek == 0 && fraction > 21 * PARTS_PER_HOUR + 589 && isLeapYear(year, CURRENT_ERA))
      day++;
    return day;
  }

  private DateTime getGregorianDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    int yearType = getYearType(year);
    int days = getStartOfYear(year) - DAYS_TO_1AD + day - 1;
    for (int i = 1; i <= month; i++)
      days += MONTHDAYS[yearType][i - 1];
    return DateTime((days * TICKS_PER_DAY) + TimeSpan.getTicks(hour, minute, second) + (millisecond * TICKS_PER_MILLISECOND));
  }

}

package const int[] DAYS_TO_MONTH_COMMON = [ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 ];
package const int[] DAYS_TO_MONTH_LEAP = [ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 ];

package const ulong TICKS_PER_MILLISECOND = 10000;
package const ulong TICKS_PER_SECOND = TICKS_PER_MILLISECOND * 1000;
package const ulong TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;
package const ulong TICKS_PER_HOUR = TICKS_PER_MINUTE * 60;
package const ulong TICKS_PER_DAY = TICKS_PER_HOUR * 24;

package const int MILLIS_PER_SECOND = 1000;
package const int MILLIS_PER_MINUTE = MILLIS_PER_SECOND * 60;
package const int MILLIS_PER_HOUR = MILLIS_PER_MINUTE * 60;
package const int MILLIS_PER_DAY = MILLIS_PER_HOUR * 24;

package const int DAYS_PER_YEAR = 365;
package const int DAYS_PER_4_YEARS = DAYS_PER_YEAR * 4 + 1;
package const int DAYS_PER_100_YEARS = DAYS_PER_4_YEARS * 25 - 1;
package const int DAYS_PER_400_YEARS = DAYS_PER_100_YEARS * 4 + 1;

package const int DAYS_TO_1601 = DAYS_PER_400_YEARS * 4;
package const int DAYS_TO_10000 = DAYS_PER_400_YEARS * 25 - 366;

package void splitDate(ulong ticks, out int year, out int month, out int day, out int dayOfYear) {
  int numDays = cast(int)(ticks / TICKS_PER_DAY);
  int whole400Years = numDays / DAYS_PER_400_YEARS;
  numDays -= whole400Years * DAYS_PER_400_YEARS;
  int whole100Years = numDays / DAYS_PER_100_YEARS;
  if (whole100Years == 4)
    whole100Years = 3;
  numDays -= whole100Years * DAYS_PER_100_YEARS;
  int whole4Years = numDays / DAYS_PER_4_YEARS;
  numDays -= whole4Years * DAYS_PER_4_YEARS;
  int wholeYears = numDays / DAYS_PER_YEAR;
  if (wholeYears == 4)
    wholeYears = 3;
  year = whole400Years * 400 + whole100Years * 100 + whole4Years * 4 + wholeYears + 1;
  numDays -= wholeYears * DAYS_PER_YEAR;
  dayOfYear = numDays + 1;
  int[] monthDays = (wholeYears == 3 && (whole4Years != 24 || whole100Years == 3)) ? DAYS_TO_MONTH_LEAP : DAYS_TO_MONTH_COMMON;
  month = numDays >> 5 + 1;
  while (numDays >= monthDays[month])
    month++;
  day = numDays - monthDays[month - 1] + 1;
}

package int extractPart(ulong ticks, DatePart part) {
  int year, month, day, dayOfYear;
  splitDate(ticks, year, month, day, dayOfYear);
  if (part == DatePart.YEAR)
    return year;
  else if (part == DatePart.MONTH)
    return month;
  else if (part == DatePart.DAY_OF_YEAR)
    return dayOfYear;
  return day;
}

/**
 * $(ANCHOR _DateTime)
 * Represents time expressed as a date and time of day.
 * Remarks: DateTime respresents dates and times between 12:00:00 midnight on January 1, 0001 AD and 11:59:59 PM on 
 * December 31, 9999 AD.
 *
 * Time values are measured in 100-nanosecond intervals, or ticks. A date value is the number of ticks that have elapsed since 
 * 12:00:00 midnight on January 1, 0001 AD in the $(LINK2 #GregorianCalendar, GregorianCalendar) calendar.
 */
public struct DateTime {

  package enum Kind : ulong {
    UNKNOWN = 0x0000000000000000,
    UTC = 0x4000000000000000,
    LOCAL = 0x8000000000000000
  }

  private const ulong MIN_TICKS = 0;
  private const ulong MAX_TICKS = DAYS_TO_10000 * TICKS_PER_DAY - 1;

  private const ulong TICKS_MASK = 0x3FFFFFFFFFFFFFFF;
  private const ulong KIND_MASK = 0xC000000000000000;

  private const int KIND_SHIFT = 62;

  private ulong data_;

  /**
   * Represents the smallest DateTime value.
   */
  public static const DateTime min;
  /**
   * Represents the largest DateTime value.
   */
  public static const DateTime max;

  static this() {
    min = DateTime(MIN_TICKS);
    max = DateTime(MAX_TICKS);
  }

  /**
   * $(I Constructor.) Initializes a new instance of the DateTime struct to the specified number of _ticks.
   * Params: ticks = A date and time expressed in units of 100 nanoseconds.
   */
  public static DateTime opCall(ulong ticks) {
    DateTime d;
    d.data_ = ticks;
    return d;
  }

  /**
   * $(I Constructor.) Initializes a new instance of the DateTime struct to the specified _year, _month, _day and _calendar.
   * Params:
   *   year = The _year.
   *   month = The _month.
   *   day = The _day.
   *   calendar = The Calendar that applies to this instance.
   */
  public static DateTime opCall(int year, int month, int day, Calendar calendar = null) {
    DateTime d;
    if (calendar is null)
      d.data_ = getDateTicks(year, month, day);
    else
      d.data_ = calendar.getDateTime(year, month, day, 0, 0, 0, 0).ticks;
    return d;
  }

  /**
   * $(I Constructor.) Initializes a new instance of the DateTime struct to the specified _year, _month, _day, _hour, _minute, _second and _calendar.
   * Params:
   *   year = The _year.
   *   month = The _month.
   *   day = The _day.
   *   hour = The _hours.
   *   minute = The _minutes.
   *   second = The _seconds.
   *   calendar = The Calendar that applies to this instance.
   */
  public static DateTime opCall(int year, int month, int day, int hour, int minute, int second, Calendar calendar = null) {
    DateTime d;
    if (calendar is null)
      d.data_ = getDateTicks(year, month, day) + getTimeTicks(hour, minute, second);
    else
      d.data_ = calendar.getDateTime(year, month, day, hour, minute, second, 0).ticks;
    return d;
  }

  /**
   * $(I Constructor.) Initializes a new instance of the DateTime struct to the specified _year, _month, _day, _hour, _minute, _second, _millisecond and _calendar.
   * Params:
   *   year = The _year.
   *   month = The _month.
   *   day = The _day.
   *   hour = The _hours.
   *   minute = The _minutes.
   *   second = The _seconds.
   *   millisecond = The _milliseconds.
   *   calendar = The Calendar that applies to this instance.
   */
  public static DateTime opCall(int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar = null) {
    DateTime d;
    if (calendar is null)
      d.data_ = getDateTicks(year, month, day) + getTimeTicks(hour, minute, second) + (millisecond * TICKS_PER_MILLISECOND);
    else
      d.data_ = calendar.getDateTime(year, month, day, hour, minute, second, millisecond).ticks;
    return d;
  }

  /**
   * Compares two DateTime values.
   */
  public int opCmp(DateTime value) {
    if (ticks < value.ticks)
      return -1;
    else if (ticks > value.ticks)
      return 1;
    return 0;
  }

  /**
   * Determines whether two DateTime values are equal.
   * Params: value = A DateTime _value.
   * Returns: true if both instances are equal; otherwise, false;
   */
  public bool opEquals(DateTime value) {
    return ticks == value.ticks;
  }

  /**
   * Adds the specified time span to the date and time, returning a new date and time.
   * Params: t = A TimeSpan value.
   * Returns: A DateTime that is the sum of this instance and t.
   */
  public DateTime opAdd(TimeSpan t) {
    return DateTime(ticks + t.ticks);
  }

  /**
   * Adds the specified time span to the date and time, assigning the result to this instance.
   * Params: t = A TimeSpan value.
   * Returns: The current DateTime instance, with t added to the date and time.
   */
  public DateTime opAddAssign(TimeSpan t) {
    return data_ = (ticks + t.ticks) | kind, *this;
  }

  /**
   * Subtracts the specified time span from the date and time, returning a new date and time.
   * Params: t = A TimeSpan value.
   * Returns: A DateTime whose value is the value of this instance minus the value of t.
   */
  public DateTime opSub(TimeSpan t) {
    return DateTime(ticks - t.ticks);
  }

  /**
   * Subtracts the specified time span from the date and time, assigning the result to this instance.
   * Params: t = A TimeSpan value.
   * Returns: The current DateTime instance, with t subtracted from the date and time.
   */
  public DateTime opSubAssign(TimeSpan t) {
    return data_ = (ticks - t.ticks) | kind, *this;
  }

  /**
   * Adds the specified number of ticks to the _value of this instance.
   * Params: value = The number of ticks to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the time in value.
   */
  public DateTime addTicks(ulong value) {
    return DateTime((ticks + value) | kind);
  }

  /**
   * Adds the specified number of hours to the _value of this instance.
   * Params: value = The number of hours to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of hours in value.
   */
  public DateTime addHours(int value) {
    return addMilliseconds(value * MILLIS_PER_HOUR);
  }

  /**
   * Adds the specified number of minutes to the _value of this instance.
   * Params: value = The number of minutes to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of minutes in value.
   */
  public DateTime addMinutes(int value) {
    return addMilliseconds(value * MILLIS_PER_MINUTE);
  }

  /**
   * Adds the specified number of seconds to the _value of this instance.
   * Params: value = The number of seconds to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of seconds in value.
   */
  public DateTime addSeconds(int value) {
    return addMilliseconds(value * MILLIS_PER_SECOND);
  }

  /**
   * Adds the specified number of milliseconds to the _value of this instance.
   * Params: value = The number of milliseconds to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of milliseconds in value.
   */
  public DateTime addMilliseconds(double value) {
    return addTicks(cast(ulong)(cast(long)(value + ((value >= 0) ? 0.5 : -0.5))) * TICKS_PER_MILLISECOND);
  }

  /**
   * Adds the specified number of days to the _value of this instance.
   * Params: value = The number of days to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of days in value.
   */
  public DateTime addDays(int value) {
    return addMilliseconds(value * MILLIS_PER_DAY);
  }

  /**
   * Adds the specified number of months to the _value of this instance.
   * Params: value = The number of months to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of months in value.
   */
  public DateTime addMonths(int value) {
    int year = this.year;
    int month = this.month;
    int day = this.day;
    int n = month - 1 + value;
    if (n >= 0) {
      month = n % 12 + 1;
      year = year + n / 12;
    }
    else {
      month = 12 + (n + 1) % 12;
      year = year + (n - 11) / 12;
    }
    int maxDays = daysInMonth(year, month);
    if (day > maxDays)
      day = maxDays;
    return DateTime((getDateTicks(year, month, day) + (ticks % TICKS_PER_DAY)) | kind);
  }

  /**
   * Adds the specified number of years to the _value of this instance.
   * Params: value = The number of years to add.
   * Returns: A DateTime whose value is the sum of the date and time of this instance and the number of years in value.
   */
  public DateTime addYears(int value) {
    return addMonths(value * 12);
  }

  /**
   * Returns the number of _days in the specified _month.
   * Params:
   *   year = The _year.
   *   month = The _month.
   * Returns: The number of _days in the specified _month.
   */
  public static int daysInMonth(int year, int month) {
    int[] monthDays = isLeapYear(year) ? DAYS_TO_MONTH_LEAP : DAYS_TO_MONTH_COMMON;
    return monthDays[month] - monthDays[month - 1];
  }

  /**
   * Returns a value indicating whether the specified _year is a leap _year.
   * Param: year = The _year.
   * Returns: true if year is a leap _year; otherwise, false.
   */
  public static bool isLeapYear(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  }

  /**
   * Determines whether this instance is within the Daylight Saving Time range for the current time zone.
   * Returns: true if the value of this instance is within the Daylight Saving Time for the current time zone; otherwise, false.
   */
  public bool isDaylightSavingTime() {
    return TimeZone.current.isDaylightSavingTime(*this);
  }

  /**
   * Converts the value of this instance to local time.
   * Returns: A DateTime whose value is the local time equivalent to the value of the current instance.
   */
  public DateTime toLocalTime() {
    return TimeZone.current.getLocalTime(*this);
  }

  /**
   * Converts the value of this instance to UTC time.
   * Returns: A DateTime whose value is the UTC equivalent to the value of the current instance.
   */
  public DateTime toUniversalTime() {
    return TimeZone.current.getUniversalTime(*this);
  }
  
  /**
   * Converts the value of this instance to its equivalent string representation using the specified culture-specific formatting information.
   * Params: formatService = An IFormatService that provides culture-specific formatting information.
   * Returns: A string representation of the value of this instance as specified by formatService.
   * Remarks: The value of the DateTime instance is formatted using the "G" format specifier.
   *
   * See $(LINK2 datetimeformat.html, DateTime Formatting) for more information about date and time formatting.
   */
  public char[] toUtf8(IFormatService formatService = null) {
    return formatDateTime(*this, null, DateTimeFormat.getInstance(formatService));
  }

  /**
   * Converts the value of this instance to its equivalent string representation using the specified _format and culture-specific formatting information.
   * Params: 
   *   format = A _format string.
   *   formatService = An IFormatService that provides culture-specific formatting information.
   * Returns: A string representation of the value of this instance as specified by format and formatService.
   * Remarks: See $(LINK2 datetimeformat.html, DateTime Formatting) for more information about date and time formatting.
   * Examples:
   * ---
   * import tango.io.Print, tango.text.locale.Core;
   *
   * void main() {
   *   Culture culture = Culture.current;
   *   DateTime now = DateTime.now;
   *
   *   Println("Current date and time: %s", now.toUtf8());
   *   Println();
   *
   *   // Format the current date and time in a number of ways.
   *   Println("Culture: %s", culture.englishName);
   *   Println();
   *
   *   Println("Short date:              %s", now.toUtf8("d"));
   *   Println("Long date:               %s", now.toUtf8("D"));
   *   Println("Short time:              %s", now.toUtf8("t"));
   *   Println("Long time:               %s", now.toUtf8("T"));
   *   Println("General date short time: %s", now.toUtf8("g"));
   *   Println("General date long time:  %s", now.toUtf8("G"));
   *   Println("Month:                   %s", now.toUtf8("M"));
   *   Println("RFC1123:                 %s", now.toUtf8("R"));
   *   Println("Sortable:                %s", now.toUtf8("s"));
   *   Println("Year:                    %s", now.toUtf8("Y"));
   *   Println();
   *
   *   // Display the same values using a different culture.
   *   culture = Culture.getCulture("fr-FR");
   *   Println("Culture: %s", culture.englishName);
   *   Println();
   *
   *   Println("Short date:              %s", now.toUtf8("d", culture));
   *   Println("Long date:               %s", now.toUtf8("D", culture));
   *   Println("Short time:              %s", now.toUtf8("t", culture));
   *   Println("Long time:               %s", now.toUtf8("T", culture));
   *   Println("General date short time: %s", now.toUtf8("g", culture));
   *   Println("General date long time:  %s", now.toUtf8("G", culture));
   *   Println("Month:                   %s", now.toUtf8("M", culture));
   *   Println("RFC1123:                 %s", now.toUtf8("R", culture));
   *   Println("Sortable:                %s", now.toUtf8("s", culture));
   *   Println("Year:                    %s", now.toUtf8("Y", culture));
   *   Println();
   * }
   *
   * // Produces the following output:
   * // Current date and time: 26/05/2006 10:04:57 AM
   * //
   * // Culture: English (United Kingdom)
   * //
   * // Short date:              26/05/2006
   * // Long date:               26 May 2006
   * // Short time:              10:04
   * // Long time:               10:04:57 AM
   * // General date short time: 26/05/2006 10:04
   * // General date long time:  26/05/2006 10:04:57 AM
   * // Month:                   26 May
   * // RFC1123:                 Fri, 26 May 2006 10:04:57 GMT
   * // Sortable:                2006-05-26T10:04:57
   * // Year:                    May 2006
   * //
   * // Culture: French (France)
   * //
   * // Short date:              26/05/2006
   * // Long date:               vendredi 26 mai 2006
   * // Short time:              10:04
   * // Long time:               10:04:57
   * // General date short time: 26/05/2006 10:04
   * // General date long time:  26/05/2006 10:04:57
   * // Month:                   26 mai
   * // RFC1123:                 ven., 26 mai 2006 10:04:57 GMT
   * // Sortable:                2006-05-26T10:04:57
   * // Year:                    mai 2006
   * ---
   */
  public char[] toUtf8(char[] format, IFormatService formatService = null) {
    return formatDateTime(*this, format, DateTimeFormat.getInstance(formatService));
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalent using the specified culture-specific formatting information.
   * Params:
   *   s = A string representing the date and time to convert.
   *   formatService = An IFormatService that provides culture-specific formatting information.
   * Returns: A DateTime equivalent to the date and time contained in s as specified by formatService.
   * Remarks: The s parameter is parsed using the formatting information in the current $(LINK2 #DateTimeFormat, DateTimeFormat) instance.
   * Examples:
   * ---
   * import tango.io.Print, tango.text.locale.Core;
   *
   * void main() {
   *   // Date is May 26, 2006
   *   char[] ukDateValue = "26/05/2006 9:15:10";
   *   char[] usDateValue = "05/26/2006 9:15:10";
   *
   *   Culture.current = Culture.getCulture("en-GB");
   *   DateTime ukDate = DateTime.parse(ukDateValue);
   *   Println("UK date: %s", ukDate.toUtf8());

   *   Culture.current = Culture.getCulture("en-US");
   *   DateTime usDate = DateTime.parse(usDateValue);
   *   Println("US date: %s", usDate.toUtf8());
   * }
   *
   * // Produces the following output:
   * // UK date: 26/05/2006 9:15:10 AM
   * // US date: 5/26/2006 9:15:10 AM
   * ---
   */
  public static DateTime parse(char[] s, IFormatService formatService = null) {
    DateTime result = parseDateTime(s, DateTimeFormat.getInstance(formatService));
    return result;
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalent using the specified culture-specific formatting information.
   * The _format of the string representation must exactly match the specified format.
   * Params:
   *   s = A string representing the date and time to convert.
   *   format = The expected _format of s.
   *   formatService = An IFormatService that provides culture-specific formatting information.
   * Returns: A DateTime equivalent to the date and time contained in s as specified by format and formatService.
   */
  public static DateTime parseExact(char[] s, char[] format, IFormatService formatService = null) {
    DateTime result = parseDateTimeExact(s, format, DateTimeFormat.getInstance(formatService));
    return result;
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalant.
   * Params:
   *   s = A string representing the date and time to convert.
   *   result = On return, contains the DateTime value equivalent to the date and time in s if the conversion was successful.
   * Returns: true if s is converted successfully; otherwise, false.
   */
  public static bool tryParse(char[] s, out DateTime result) {
    return tryParseDateTime(s, DateTimeFormat.current, result);
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalant using the specified culture-specific formatting information.
   * Params:
   *   s = A string representing the date and time to convert.
   *   formatService = An IFormatService that provides culture-specific formatting information.
   *   result = On return, contains the DateTime value equivalent to the date and time in s if the conversion was successful.
   * Returns: true if s is converted successfully; otherwise, false.
   */
  public static bool tryParse(char[] s, IFormatService formatService, out DateTime result) {
    return tryParseDateTime(s, DateTimeFormat.getInstance(formatService), result);
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalant using the specified culture-specific formatting information.
   * The _format of the string representation must exactly match the specified format.
   * Params:
   *   s = A string representing the date and time to convert.
   *   format = The expected _format of s.
   *   result = On return, contains the DateTime value equivalent to the date and time in s if the conversion was successful.
   * Returns: true if s is converted successfully; otherwise, false.
   */
  public static bool tryParseExact(char[] s, char[] format, out DateTime result) {
    return tryParseDateTimeExact(s, format, DateTimeFormat.current, result);
  }

  /**
   * Converts the specified string representation of a date and time to its DateTime equivalant using the specified culture-specific formatting information.
   * The _format of the string representation must exactly match the specified format.
   * Params:
   *   s = A string representing the date and time to convert.
   *   format = The expected _format of s.
   *   formatService = An IFormatService that provides culture-specific formatting information.
   *   result = On return, contains the DateTime value equivalent to the date and time in s if the conversion was successful.
   * Returns: true if s is converted successfully; otherwise, false.
   */
  public static bool tryParseExact(char[] s, char[] format, IFormatService formatService, out DateTime result) {
    return tryParseDateTimeExact(s, format, DateTimeFormat.getInstance(formatService), result);
  }

  /**
   * $(I Property.) Retrieves the _year component of the date.
   * Returns: The _year.
   */
  public int year() {
    return extractPart(ticks, DatePart.YEAR);
  }

  /**
   * $(I Property.) Retrieves the _month component of the date.
   * Returns: The _month.
   */
  public int month() {
    return extractPart(ticks, DatePart.MONTH);
  }

  /**
   * $(I Property.) Retrieves the _day component of the date.
   * Returns: The _day.
   */
  public int day() {
    return extractPart(ticks, DatePart.DAY);
  }

  /**
   * $(I Property.) Retrieves the day of the year.
   * Returns: The day of the year.
   */
  public int dayOfYear() {
    return extractPart(ticks, DatePart.DAY_OF_YEAR);
  }

  /**
   * $(I Property.) Retrieves the day of the week.
   * Returns: A DayOfWeek value indicating the day of the week.
   */
  public DayOfWeek dayOfWeek() {
    return cast(DayOfWeek)((ticks / TICKS_PER_DAY + 1) % 7);
  }

  /**
   * $(I Property.) Retrieves the _hour component of the date.
   * Returns: The _hour.
   */
  public int hour() {
    return cast(int)((ticks / TICKS_PER_HOUR) % 24);
  }

  /**
   * $(I Property.) Retrieves the _minute component of the date.
   * Returns: The _minute.
   */
  public int minute() {
    return cast(int)((ticks / TICKS_PER_MINUTE) % 60);
  }

  /**
   * $(I Property.) Retrieves the _second component of the date.
   * Returns: The _second.
   */
  public int second() {
    return cast(int)((ticks / TICKS_PER_SECOND) % 60);
  }

  /**
   * $(I Property.) Retrieves the _millisecond component of the date.
   * Returns: The _millisecond.
   */
  public int millisecond() {
    return cast(int)((ticks / TICKS_PER_MILLISECOND) % 1000);
  }

  /**
   * $(I Property.) Retrieves the date component.
   * Returns: A new DateTime instance with the same date as this instance.
   */
  public DateTime date() {
    ulong ticks = this.ticks;
    return DateTime((ticks - ticks % TICKS_PER_DAY) | kind);
  }

  /**
   * $(I Property.) Retrieves the current date.
   * Returns: A DateTime instance set to today's date.
   */
  public static DateTime today() {
    // return now.date;
    // The above code causes DMD to complain about lvalues in toLocalTime, so we need a temporary here.
    DateTime d = now;
    return d.date;
  }

  /**
   * $(I Property.) Retrieves the time of day.
   * Returns: A TimeSpan representing the fraction of the day elapsed since midnight.
   */
  public TimeSpan timeOfDay() {
    return TimeSpan(ticks % TICKS_PER_DAY);
  }

  /**
   * $(I Property.) Retrieves the number of ticks representing the date and time of this instance.
   * Returns: The number of ticks representing the date and time of this instance.
   */
  public ulong ticks() {
    return data_ & TICKS_MASK;
  }

  /**
   * $(I Property.) Retrieves a DateTime instance set to the current date and time in local time.
   * Returns: A DateTime whose value is the current local date and time.
   * Examples:
   * The following example displays the current time in local and UTC time.
   * ---
   * import tango.io.Print, tango.text.locale.Core;
   *
   * void main() {
   *   // Get the current local time.
   *   DateTime localTime = DateTime.now;
   *
   *   // Convert the current local time to UTC time.
   *   DateTime utcTime = localTime.toUniversalTime();
   *
   *   // Display the local and UTC time using a custom pattern.
   *   char[] pattern = "d/M/yyyy hh:mm:ss tt";
   *   Println("Local time: %s", localTime.toUtf8(pattern));
   *   Println("UTC time:   %s", utcTime.toUtf8(pattern));
   * }
   *
   * // Produces the following output:
   * // Local time: 26/5/2006 9:15:00 AM
   * // UTC time:   26/5/2006 8:15:00 AM
   * ---
   */
  public static DateTime now() {
    // return utcNow.toLocalTime();
    // The above code causes DMD to complain about lvalues in toLocalTime, so we need a temporary here.
    DateTime d = utcNow.toLocalTime();
    return d;
  }

  /**
   * $(I Property.) Retrieves a DateTime instance set to the current date and time in UTC time.
   * Returns: A DateTime whose value is the current UTC date and time.
   */
  public static DateTime utcNow() {
    ulong ticks = nativeMethods.getUtcTime();
    return DateTime(cast(ulong)(ticks + (DAYS_TO_1601 * TICKS_PER_DAY)) | Kind.UTC);
  }

  package ulong kind() {
    return data_ & KIND_MASK;
  }

  private static ulong getDateTicks(int year, int month, int day) {
    int[] monthDays = isLeapYear(year) ? DAYS_TO_MONTH_LEAP : DAYS_TO_MONTH_COMMON;
    year--;
    return (year * 365 + year / 4 - year / 100 + year / 400 + monthDays[month - 1] + day - 1) * TICKS_PER_DAY;
  }

  private static ulong getTimeTicks(int hour, int minute, int second) {
    return (cast(ulong)hour * 3600 + cast(ulong)minute * 60 + cast(ulong)second) * TICKS_PER_SECOND;
  }

}

/**
 * $(ANCHOR _TimeSpan)
 * Represents a time interval.
 */
public struct TimeSpan {

  private ulong ticks_;

  /**
   * Represents the minimum value.
   */
  public static const TimeSpan min;
  /**
   * Represents the maximum value.
   */
  public static const TimeSpan max;

  static this() {
    min = TimeSpan(ulong.min);
    max = TimeSpan(ulong.max);
  }

  /**
   * $(I Constructor.) Initializes a new TimeSpan to the specified number of _ticks.
   * Params: ticks = A time interval in units of 100 nanoseconds.
   */
  public static TimeSpan opCall(ulong ticks) {
    TimeSpan t;
    t.ticks_ = ticks;
    return t;
  }

  /**
   * $(I Constructor.) Initializes a new TimeSpan to the specified number of _hours, _minutes and _seconds.
   * Params:
   *   hours = The number of _hours.
   *   minutes = The number of _minutes.
   *   seconds = The number of _seconds.
   */
  public static TimeSpan opCall(int hours, int minutes, int seconds) {
    TimeSpan t;
    t.ticks_ = getTicks(hours, minutes, seconds);
    return t;
  }

  /**
   * $(I Constructor.) Initializes a new TimeSpan to the specified number of _hours, _minutes, _seconds and _milliseconds.
   * Params:
   *   hours = The number of _hours.
   *   minutes = The number of _minutes.
   *   seconds = The number of _seconds.
   *   milliseconds = The number of _milliseconds.
   */
  public static TimeSpan opCall(int hours, int minutes, int seconds, int milliseconds) {
    TimeSpan t;
    t.ticks_ = getTicks(hours, minutes, seconds) + (milliseconds * TICKS_PER_MILLISECOND);
    return t;
  }

  /**
   * Adds two TimeSpan instances.
   * Params: t = A TimeSpan.
   * Returns: A TimeSpan whose value is the sum of the value of this instance and the value of t.
   */
  public TimeSpan opAdd(TimeSpan t) {
    return TimeSpan(ticks_ + t.ticks_);
  }

  /**
   * Adds two TimeSpan instances and assigns the result to this instance.
   * Params: t = A TimeSpan.
   * Returns: This instance, modified by adding value of t.
   */
  public TimeSpan opAddAssign(TimeSpan t) {
    ticks_ += t.ticks_;
    return *this;
  }

  /**
   * Subtracts the specified TimeSpan from this instance.
   * Params: t = A TimeSpan.
   * Returns: A TimeSpan whose value is the result of the value of this instance minus the value of t.
   */
  public TimeSpan opSub(TimeSpan t) {
    return TimeSpan(ticks_ - t.ticks_);
  }

  /**
   * Subtracts the specified TimeSpan and assigns the result to this instance.
   * Params: t = A TimeSpan.
   * Returns: This instance, modified by subtracting the value of t.
   */
  public TimeSpan opSubAssign(TimeSpan t) {
    ticks_ -= t.ticks_;
    return *this;
  }

  /**
   * Returns a TimeSpan whose value is the negated value of this instance.
   * Returns: The same value as this instance with the opposite sign.
   */
  public TimeSpan negate() {
    return TimeSpan(-ticks_);
  }

  /**
   * $(I Property.) Retrieves the number of _ticks representing the value of this instance.
   * Returns: The number of _ticks in this instance.
   */
  public ulong ticks() {
    return ticks_;
  }

  /**
   * $(I Property.) Retrieves the number of _hours represented by this instance.
   * Returns: The number of _hours in this instance.
   */
  public int hours() {
    return cast(int)((ticks_ / TICKS_PER_HOUR) % 24);
  }

  /**
   * $(I Property.) Retrieves the number of _minutes represented by this instance.
   * Returns: The number of _minutes in this instance.
   */
  public int minutes() {
    return cast(int)((ticks_ / TICKS_PER_MINUTE) % 60);
  }

  /**
   * $(I Property.) Retrieves the number of _seconds represented by this instance.
   * Returns: The number of _seconds in this instance.
   */
  public int seconds() {
    return cast(int)((ticks_ / TICKS_PER_SECOND) % 60);
  }

  /**
   * $(I Property.) Retrieves the number of _milliseconds represented by this instance.
   * Returns: The number of _milliseconds in this instance.
   */
  public int milliseconds() {
    return cast(int)((ticks_ / TICKS_PER_MILLISECOND) % 1000);
  }

  /**
   * $(I Property.) Retrieves the number of _days represented by this instance.
   * Returns: The number of _days in this instance.
   */
  public int days() {
    return cast(int)(ticks_ / TICKS_PER_DAY);
  }

  private static ulong getTicks(int hour, int minute, int second) {
    return (cast(ulong)hour * 3600 + cast(ulong)minute * 60 + cast(ulong)second) * TICKS_PER_SECOND;
  }

}

/**
 * $(ANCHOR _DaylightSavingTime)
 * Represents a period of daylight-saving time.
 */
public class DaylightSavingTime {

  private DateTime start_;
  private DateTime end_;
  private TimeSpan change_;

  /**
   * Initializes a new instance of the DaylightSavingTime class.
   * Params:
   *   start = The DateTime representing the date and time when the daylight-saving period starts.
   *   end = The DateTime representing the date and time when the daylight-saving period ends.
   *   change = The TimeSpan representing the difference between the standard time and daylight-saving time.
   */
  public this(DateTime start, DateTime end, TimeSpan change) {
    start_ = start;
    end_ = end;
    change_ = change;
  }

  /**
   * $(I Property.) Retrieves the DateTime representing the date and time when the daylight-saving period starts.
   * Returns: The DateTime representing the date and time when the daylight-saving period starts.
   */
  public DateTime start() {
    return start_;
  }

  /**
   * $(I Property.) Retrieves the DateTime representing the date and time when the daylight-saving period ends.
   * Returns: The DateTime representing the date and time when the daylight-saving period ends.
   */
  public DateTime end() {
    return end_;
  }

  /**
   * $(I Property.) Retrieves the TimeSpan representing the difference between the standard time and daylight-saving time.
   * Returns: The TimeSpan representing the difference between the standard time and daylight-saving time.
   */
  public TimeSpan change() {
    return change_;
  }

}

/**
 * $(ANCHOR _TimeZone);
 * Represents the current time zone.
 */
public class TimeZone {

  private static TimeZone current_;
  private static DaylightSavingTime[int] changesCache_;
  private short[] changesData_;
  private ulong ticksOffset_;

  /**
   * Returns the daylight-saving period for the specified _year.
   * Params: year = The _year to which the daylight-saving period applies.
   * Returns: A DaylightSavingTime instance containing the start and end for daylight saving in year.
   */
  public DaylightSavingTime getDaylightChanges(int year) {
      
     DateTime getSunday(int year, int month, int day, int sunday, int hour, int minute, int second, int millisecond) {
        DateTime result;
        if (sunday > 4) {
          result = DateTime(year, month, GregorianCalendar.getDefaultInstance().getDaysInMonth(year, month), hour, minute, second, millisecond);
          int change = cast(int)result.dayOfWeek - day;
          if (change < 0)
            change += 7;
          if (change > 0)
            result = result.addDays(-change);
        }
        else {
         result = DateTime(year, month, 1, hour, minute, second, millisecond);
         int change = day - cast(int)result.dayOfWeek;
         if (change < 0)
           change += 7;
         change += 7 * (sunday - 1);
         if (change > 0)
           result = result.addDays(change);
        }
        return result;
     }
  
     if (!(year in changesCache_)) {
       if (changesData_ == null)
         changesCache_[year] = new DaylightSavingTime(DateTime.min, DateTime.max, TimeSpan.init);
       else
         changesCache_[year] = new DaylightSavingTime(getSunday(year, changesData_[1], changesData_[2], changesData_[3], changesData_[4], changesData_[5], changesData_[6], changesData_[7]), getSunday(year, changesData_[9], changesData_[10], changesData_[11], changesData_[12], changesData_[13], changesData_[14], changesData_[15]), TimeSpan(changesData_[16] * TICKS_PER_MINUTE));
     }
     return changesCache_[year];
   }

  /**
   * Returns the local _time representing the specified UTC _time.
   * Params: time = A UTC _time.
   * Returns: A DateTime whose value is the local _time corresponding to time.
   */
  public DateTime getLocalTime(DateTime time) {
    if (time.kind == DateTime.Kind.LOCAL)
      return time;
    TimeSpan offset = TimeSpan(ticksOffset_);
    DaylightSavingTime dst = getDaylightChanges(time.year);
    if (dst.change.ticks != 0) {
      DateTime start = dst.start - offset;
      DateTime end = dst.end - offset - dst.change;
      bool isDst = (start > end) ? (time < end || time >= start) : (time >= start && time < end);
      if (isDst)
        offset += dst.change;
    }
    return DateTime((time.ticks + offset.ticks) | DateTime.Kind.LOCAL);
  }

  /**
   * Returns the UTC _time representing the specified locale _time.
   * Params: time = A UTC time.
   * Returns: A DateTime whose value is the UTC time corresponding to time.
   */
  public DateTime getUniversalTime(DateTime time) {
    if (time.kind == DateTime.Kind.UTC)
      return time;
    return DateTime((time.ticks - getUtcOffset(time).ticks) | DateTime.Kind.UTC);
  }

  /**
   * Returns the UTC _time offset for the specified local _time.
   * Params: time = The local _time.
   * Returns: The UTC offset from time.
   */
  public TimeSpan getUtcOffset(DateTime time) {
    TimeSpan offset;
    if (time.kind != DateTime.Kind.UTC) {
      DaylightSavingTime dst = getDaylightChanges(time.year);
      DateTime start = dst.start + dst.change;
      DateTime end = dst.end;
      bool isDst = (start > end) ? (time >= start || time < end) : (time >= start && time < end);
      if (isDst)
        offset = dst.change;
    }
    return TimeSpan(offset.ticks + ticksOffset_);
  }

  /**
   * Returns a value indicating whether the specified date and _time is within a daylight-saving period.
   * Params: time = A date and _time.
   * Returns: true if time is within a daylight-saving period; otherwise, false.
   */
  public bool isDaylightSavingTime(DateTime time) {
    return getUtcOffset(time) != TimeSpan.init;
  }

  /**
   * $(I Property.) Retrieves the _current time zone.
   * Returns: A TimeZone instance representing the _current time zone.
   */
  public static TimeZone current() {
    if (current_ is null)
      current_ = new TimeZone;
    return current_;
  }

  private this() {
    changesData_ = nativeMethods.getDaylightChanges();
    if (changesData_ != null)
      ticksOffset_ = changesData_[17] * TICKS_PER_MINUTE;
  }

}
