/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.AppendFiles;

private import  tango.time.Time;

private import  Path = tango.io.Path,
                tango.io.device.File;

private import  tango.io.model.IFile;

private import  tango.util.log.Log,
                tango.util.log.AppendFile;

/*******************************************************************************

        Append log messages to a file set. 

*******************************************************************************/

public class AppendFiles : Filer
{
        private Mask            mask_;
        private char[][]        paths;
        private int             index;
        private long            maxSize,
                                fileSize;

        /***********************************************************************
                
                Create a RollingFileAppender upon a file-set with the 
                specified path and optional layout. The minimal file
                count is two and the maximum is 1000. Note that files
                are numbered starting with zero rather than one.

                Where a file set already exists, we resume appending to 
                the one with the most recent activity timestamp.

        ***********************************************************************/

        this (char[] path, int count, long maxSize, Appender.Layout how = null)
        {
                --count;
                assert (path);
                assert (count > 0 && count < 1000);

                // Get a unique fingerprint for this instance
                mask_ = register (path);

                char[3] x;
                Time mostRecent;

                for (int i=0; i <= count; ++i)
                    {
                    x[0] = cast(char)('0' + i/100);
                    x[1] = cast(char)('0' + i/10%10);
                    x[2] = cast(char)('0' + i%10);
                    auto c = Path.parse (path);
                    auto p = c.toString[0..$-c.suffix.length] ~ x ~ c.suffix;
                    paths ~= p;

                    // use the most recent file in the set
                    if (Path.exists(p))
                       {
                       auto modified = Path.modified(p);
                       if (modified > mostRecent)
                          {
                          mostRecent = modified;
                          index = i;
                          }
                       }
                    }

                // remember the maximum size 
                this.maxSize = maxSize;

                // adjust index and open the appropriate log file
                --index; 
                nextFile (false);

                // set provided layout (ignored when null)
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
                char[] msg;

                // file already full?
                if (fileSize >= maxSize)
                    nextFile (true);

                size_t write (void[] content)
                {
                        fileSize += content.length;
                        return buffer.write (content);
                }

                // write log message and flush it
                layout.format (event, &write);
                write (FileConst.NewlineString);
                buffer.flush; 
        }

        /***********************************************************************
                
                Switch to the next file within the set

        ***********************************************************************/

        private void nextFile (bool reset)
        {
                // select next file in the set
                if (++index >= paths.length)
                    index = 0;
                
                // close any existing conduit
                close;

                // make it shareable for read
                auto style = File.WriteAppending;
                style.share = File.Share.Read;
                auto conduit = new File (paths[index], style);

                configure (conduit);

                // reset file size
                if (reset)
                    conduit.truncate (fileSize = 0);
                else
                   fileSize = conduit.length;
        }
}

/*******************************************************************************

*******************************************************************************/

debug (AppendFiles)
{
        void main()
        {
                Log.root.add (new AppendFiles ("foo", 5, 6));
                auto log = Log.lookup ("fu.bar");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");
                log.trace ("hello {}", "world");

        }
}
