/**
 */
module tango.text.locale.Constants;

/**
 * Defines the types of cultures that can be retrieved from Culture.getCultures.
 */
public enum CultureTypes {
  Neutral = 1,             /// Refers to cultures that are associated with a language but not specific to a country or region.
  Specific = 2,            /// Refers to cultures that are specific to a country or region.
  All = Neutral | Specific /// Refers to all cultures.
}

  /**
   */
public enum DayOfWeek {
  Sunday,    /// Indicates _Sunday.
  Monday,    /// Indicates _Monday.
  Tuesday,   /// Indicates _Tuesday.
  Wednesday, /// Indicates _Wednesday.
  Thursday,  /// Indicates _Thursday.
  Friday,    /// Indicates _Friday.
  Saturday   /// Indicates _Saturday.
}

  /**
   */
public enum CalendarWeekRule {
  FirstDay,         /// Indicates that the first week of the year is the first week containing the first day of the year.
  FirstFullWeek,    /// Indicates that the first week of the year is the first full week following the first day of the year.
  FirstFourDayWeek  /// Indicates that the first week of the year is the first week containing at least four days.
}

  /**
   */
public enum GregorianCalendarTypes {
  Localized = 1,               /// Refers to the localized version of the Gregorian calendar.
  USEnglish = 2,               /// Refers to the US English version of the Gregorian calendar.
  MiddleEastFrench = 9,        /// Refers to the Middle East French version of the Gregorian calendar.
  Arabic = 10,                 /// Refers to the _Arabic version of the Gregorian calendar.
  TransliteratedEnglish = 11,  /// Refers to the transliterated English version of the Gregorian calendar.
  TransliteratedFrench = 12    /// Refers to the transliterated French version of the Gregorian calendar.
}

package enum DatePart {
  YEAR,
  MONTH,
  DAY,
  DAY_OF_YEAR
}