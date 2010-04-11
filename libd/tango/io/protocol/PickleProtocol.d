/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Jan 2007 : initial release
        
        author:         Kris 

*******************************************************************************/

module tango.io.protocol.PickleProtocol;

private import  tango.io.model.IBuffer,
                tango.io.model.IConduit;

private import  tango.io.protocol.EndianProtocol,
                tango.io.protocol.NativeProtocol;

/*******************************************************************************

*******************************************************************************/

version (BigEndian)
         private alias NativeProtocol SuperClass;
      else
         private alias EndianProtocol SuperClass;

/*******************************************************************************

*******************************************************************************/

class PickleProtocol : SuperClass
{
        /***********************************************************************

        ***********************************************************************/

        this (IBuffer buffer, bool prefix=true)
        {
                super (buffer, prefix);
        }

        /***********************************************************************

        ***********************************************************************/

        this (IConduit conduit, bool prefix=true)
        {
                super (conduit, prefix);
        }
}


/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        import tango.io.Buffer;

        unittest
        {
                int test = 0xcc55ff00;
                
                auto protocol = new PickleProtocol (new Buffer(32));
                protocol.write (&test, test.sizeof, protocol.Type.Int);

                auto ptr = protocol.buffer.slice (test.sizeof, false).ptr;
                protocol.read  (&test, test.sizeof, protocol.Type.Int);
                
                assert (test == 0xcc55ff00);
                
                version (LittleEndian)
                         assert (*cast(int*) ptr == 0x00ff55cc);
        }
}





