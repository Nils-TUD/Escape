/*******************************************************************************

        copyright:      Copyright (c) 2010 Kris. All rights reserved.

        license:        BSD style: $(LICENSE)
        
        version:        Jan 2010: Initial release
        
        author:         Kris
    
*******************************************************************************/

module tango.util.MinMax;

/*******************************************************************************

        Return the minimum of two arguments (of the same type)

*******************************************************************************/

T min(T)(T a, T b)
{
        return a < b ? a : b;
}

/*******************************************************************************

        Return the maximum of two arguments (of the same type)

*******************************************************************************/

T max(T)(T a, T b)
{
        return a > b ? a : b;
}


