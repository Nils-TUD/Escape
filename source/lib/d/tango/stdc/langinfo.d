module tango.stdc.langinfo;

// TODO temporary hack
typedef int nl_item;

enum : nl_item
{
        /* LC_TIME category: date and time formatting.  */

        /* Abbreviated days of the week. */
        ABDAY_1 = (((2) << 16) | 0),  /* Sun */
        ABDAY_2,
        ABDAY_3,
        ABDAY_4,
        ABDAY_5,
        ABDAY_6,
        ABDAY_7,

        /* Long-named days of the week. */
        DAY_1,			/* Sunday */
        DAY_2,
        DAY_3,
        DAY_4,
        DAY_5,
        DAY_6,
        DAY_7,

        /* Abbreviated month names.  */
        ABMON_1,			/* Jan */
        ABMON_2,
        ABMON_3,
        ABMON_4,
        ABMON_5,
        ABMON_6,
        ABMON_7,
        ABMON_8,
        ABMON_9,
        ABMON_10,
        ABMON_11,
        ABMON_12,

        /* Long month names.  */
        MON_1,			/* January */
        MON_2,
        MON_3,
        MON_4,
        MON_5,
        MON_6,
        MON_7,
        MON_8,
        MON_9,
        MON_10,
        MON_11,
        MON_12,

        AM_STR,			/* Ante meridiem string.  */
        PM_STR,			/* Post meridiem string.  */

        D_T_FMT,			/* Date and time format for strftime.  */
        D_FMT,			/* Date format for strftime.  */
        T_FMT,			/* Time format for strftime.  */
        T_FMT_AMPM,			/* 12-hour time format for strftime.  */

        ERA,				/* Alternate era.  */
        ERA_YEAR,			/* Year in alternate era format.  */
        ERA_D_FMT,			/* Date in alternate era format.  */
        ALT_DIGITS,			/* Alternate symbols for digits.  */
        ERA_D_T_FMT,			/* Date and time in alternate era format.  */
        ERA_T_FMT,			/* Time in alternate era format.  */
}

extern (C) char* nl_langinfo(nl_item item) {
	char*[] items = [
	   /* Abbreviated days of the week. */
	   /* ABDAY_1 */	"Sun",
	   /* ABDAY_2 */	"Mon",
	   /* ABDAY_3 */	"Tue",
	   /* ABDAY_4 */	"Wed",
	   /* ABDAY_5 */	"Thu",
	   /* ABDAY_6 */	"Fri",
	   /* ABDAY_7 */	"Sat",
	   
       /* Long-named days of the week. */
       /* DAY_1 */		"Sunday",
       /* DAY_2 */		"Monday",
       /* DAY_3 */		"Tuesday",
       /* DAY_4 */		"Wednesday",
       /* DAY_5 */		"Thursday",
       /* DAY_6 */		"Friday",
       /* DAY_7 */		"Saturday",

       /* Abbreviated month names.  */
       /* ABMON_1 */	"Jan",
       /* ABMON_2 */	"Feb",
       /* ABMON_3 */	"Mar",
       /* ABMON_4 */	"Apr",
       /* ABMON_5 */	"May",
       /* ABMON_6 */	"Jun",
       /* ABMON_7 */	"Jul",
       /* ABMON_8 */	"Aug",
       /* ABMON_9 */	"Sep",
       /* ABMON_10 */	"Oct",
       /* ABMON_11 */	"Nov",
       /* ABMON_12 */	"Dec",
       
       /* Long month names.  */
       /* MON_1 */		"January",
       /* MON_2 */		"February",
       /* MON_3 */		"March",
       /* MON_4 */		"April",
       /* MON_5 */		"May",
       /* MON_6 */		"June",
       /* MON_7 */		"July",
       /* MON_8 */		"August",
       /* MON_9 */		"September",
       /* MON_10 */		"October",
       /* MON_11 */		"November",
       /* MON_12 */		"December",
	
       /* Ante / post meridiem string.  */
       /* AM_STR */		"AM",
       /* PM_STR */		"PM",
       
       /* D_T_FMT */	"%m/%d/%y %H:%M:%S",
       /* D_FMT */		"%m/%d/%y",
       /* T_FMT */		"%H:%M:%S",
       /* T_FMT_AMPM */	"%I:%M:%S %P",

       // TODO ??
       /* ERA */		"%m/%d/%y %H:%M:%S",
       /* ERA_YEAR */	"%y",
       /* ERA_D_FMT */	"%m/%d/%y",
       /* ALT_DIGITS */	"",
       /* ERA_D_T_FMT*/	"%m/%d/%y %H:%M:%S",
       /* ERA_T_FMT */	"%H:%M:%S"
	];
	item -= 2 << 16;
	assert(item >= 0 && item < items.length);
	return items[item];
}
