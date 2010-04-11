/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004      
        version:        Rewritten to support alternate formatter; July 2006
                        Outback release: December 2006
       
        author:         Kris

*******************************************************************************/

module tango.io.protocol.DisplayWriter;

private import  tango.io.Buffer;

private import  tango.core.Vararg;

private import  tango.io.model.IConduit;

private import  tango.io.protocol.Writer;

private import  tango.text.convert.Format;

private import  tango.io.protocol.model.IProtocol;

/*******************************************************************************

        Format output suitable for presentation. DisplayWriter provides 
        a means to append formatted data to an IBuffer, and exposes 
        a convenient method of handling a variety of data types. 
        
        The DisplayWriter itself is a wrapper around the tango.text.convert 
        package, which should be used directly as desired (Integer, Float, 
        etc). The latter modules are home to a set of formatting-methods,
        making them convenient for ad-hoc application.

        Tango.text.convert also has a Sprint module for working more directly
        with text arrays.
         
*******************************************************************************/

class DisplayWriter : Writer
{
        private static TypeInfo[] revert = 
        [
                typeid(void), // byte.sizeof,
                typeid(char), // char.sizeof,
                typeid(bool), // bool.sizeof,
                typeid(byte), // byte.sizeof,
                typeid(ubyte), // ubyte.sizeof,
                typeid(wchar), // wchar.sizeof,
                typeid(short), // short.sizeof,
                typeid(ushort), // ushort.sizeof,
                typeid(dchar), // dchar.sizeof,
                typeid(int), // int.sizeof,
                typeid(uint), // uint.sizeof,
                typeid(float), // float.sizeof,
                typeid(long), // long.sizeof,
                typeid(ulong), // ulong.sizeof,
                typeid(double), // double.sizeof,
                typeid(real), // real.sizeof,
                typeid(Object), // (Object*).sizeof,
                typeid(void*), // (void*).sizeof,
        ];

        /***********************************************************************
        
                Construct a DisplayWriter upon the specified IBuffer. 

        ***********************************************************************/

        this (IBuffer buffer)
        {
                super (buffer);
                arrays = elements = &write;
        }
     
        /***********************************************************************
        
                Construct a DisplayWriter upon the specified IConduit

        ***********************************************************************/

        this (IConduit conduit)
        {
                this (new Buffer(conduit));
        }

        /***********************************************************************
        
                Intercept discrete output and convert it to printable form

        ***********************************************************************/

        private void write (void* src, uint bytes, IProtocol.Type type)
        {
                switch (type)
                       {
                       case IProtocol.Type.Utf8:
                       case IProtocol.Type.Utf16:
                       case IProtocol.Type.Utf32:
                            buffer_.append (src [0 .. bytes]);
                            break;

                       default:
                            char[128] output = void;
                            char[128] convert = void;
                            auto ti = revert [type];
                            auto result = Formatter.Result (output, convert);

                            auto width = ti.tsize();
                            assert ((bytes % width) is 0, "invalid arg[] length");

                            bool array = width < bytes;

                            if (array)
                                buffer_.append ("[");

                            while (bytes)
                                  {
                                  auto s = Formatter (result, ti, cast(va_list) src);
                                  buffer_.append (s);

                                  bytes -= width;
                                  src += width;

                                  if (bytes > 0)
                                      buffer_.append (", ");
                                  }

                            if (array)
                                buffer_.append ("]");
                            break;
                       }
        }
}
