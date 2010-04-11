/**
 * Contains modules providing information about specific locales.
 */
module tango.text.locale.Common;

// Issues: does not compile with "-cov" because of a circular dependency.
// tango.text.locale.Core and tango.text.locale.format need to import each other.

import  tango.text.locale.Constants,
        tango.text.locale.Core,
        tango.text.locale.Collation,
        tango.text.locale.Format,
        tango.text.locale.Parse;