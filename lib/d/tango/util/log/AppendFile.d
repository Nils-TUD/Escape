/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.AppendFile;

private import  tango.util.log.Log;

private import  tango.io.device.File;

private import  tango.io.stream.Buffered;

private import  tango.io.model.IFile,
                tango.io.model.IConduit;

/*******************************************************************************

        Append log messages to a file. This basic version has no rollover 
        support, so it just keeps on adding to the file. There's also a
        RollingFileAppender that may suit your needs.

*******************************************************************************/

class AppendFile : Filer
{
        private Mask    mask_;

        /***********************************************************************
                
                Create a basic FileAppender to a file with the specified 
                path.

        ***********************************************************************/

        this (char[] fp, Appender.Layout how = null)
        {
                // Get a unique fingerprint for this instance
                mask_ = register (fp);
        
                // make it shareable for read
                auto style = File.WriteAppending;
                style.share = File.Share.Read;
                configure (new File (fp, style));
                layout (how);
        }

        /***********************************************************************
                
                Return the fingerprint for this class

        ***********************************************************************/

        final Mask mask ()
        {
                return mask_;
        }

        /***********************************************************************
                
                Return the name of this class

        ***********************************************************************/

        final char[] name ()
        {
                return this.classinfo.name;
        }
                
        /***********************************************************************
                
                Append an event to the output.
                 
        ***********************************************************************/

        final synchronized void append (LogEvent event)
        {
                layout.format (event, &buffer.write);
                buffer.append (FileConst.NewlineString)
                      .flush;
        }
}


/*******************************************************************************

        Base class for file appenders

*******************************************************************************/

class Filer : Appender
{
        package Bout            buffer;
        private IConduit        conduit_;

        /***********************************************************************
                
                Return the conduit

        ***********************************************************************/

        final IConduit conduit ()
        {
                return conduit_;
        }

        /***********************************************************************
                
                Close the file associated with this Appender

        ***********************************************************************/

        final synchronized void close ()
        {
                if (conduit_)
                   {
                   conduit_.detach;
                   conduit_ = null;
                   }
        }

        /***********************************************************************
                
                Set the conduit

        ***********************************************************************/

        package final Bout configure (IConduit conduit)
        {
                // create a new buffer upon this conduit
                conduit_ = conduit;
                return (buffer = new Bout(conduit));
        }
}


