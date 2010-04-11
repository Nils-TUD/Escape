/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004      
        
        author:         Kris

*******************************************************************************/

module tango.io.protocol.model.IPayload;

public import tango.io.protocol.model.IReader,
              tango.io.protocol.model.IWriter;

/******************************************************************************

        Interface for all serializable classes. Such classes are intended
        to be transported over a network, or be frozen in a file for later
        reconstruction. 

        IPayload objects are expected to extend out across a cluster.

******************************************************************************/

interface IPayload : IWritable, IReadable
{
        /***********************************************************************
        
                Identify this serializable class via a char[]. This should
                be (per class) unique within the domain. Use version numbers 
                or similar mechanism to isolate different implementations of
                the same class.

        ***********************************************************************/

        char[] guid ();

        /***********************************************************************

                Create an instance of the payload
                
        ***********************************************************************/

        IPayload create ();
        
        /***********************************************************************

                Get the timestamp of this payload
                
        ***********************************************************************/

        ulong time ();

        /***********************************************************************

                Set the timestamp of this payload
                
        ***********************************************************************/

        void time (ulong time);

        /**********************************************************************

                Perform whatever cleanup is necessary. Could use ~this()
                instead, but we prefer it to be explicit.

        **********************************************************************/

        void destroy ();
}
