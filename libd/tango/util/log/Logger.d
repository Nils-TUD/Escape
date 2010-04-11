/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
                        Delegate support proposed by BCS: August 2006 
        
        author:         Kris

*******************************************************************************/

module tango.util.log.Logger;

private import tango.util.log.Appender;

private import tango.util.log.model.ILevel;

/*******************************************************************************

        This is the primary API to the log package. Use the two static 
        methods to access and/or create Logger instances, and the other
        methods to modify specific Logger attributes. 
        ---
        import tango.util.log.Log;
        
        auto log = Log.getLogger ("my.logger");

        log.info  ("an informational message");
        log.error ("an exception message: " ~ exception.toUtf8);

        etc ...
        ---
        
        It is considered good form to assign the logger instances during
        static construction. For example: if it were appropriate to have
        one logger instance per module, each might be assigned from within
        the module ctor
        ---
        private Logger log;
        
        static this()
        {
            log = Log.getLogger (nameOfThisModule);
        }
        ---

        Messages passed to a Logger are assumed to be pre-formatted. You 
        may find that the Sprint class is handy for collating various 
        components of the message (or use Formatter.sprint() directly): 
        ---
        static this()
        {
            sprint = new Sprint (256);
            log = Log.getLogger (nameOfThisModule);
        }

        log.warn (sprint("temperature is {0} degrees!", 101));
        ---

        To avoid overhead when constructing formatted messages, check to
        see if the logger is active first
        ---
        if (isActive (log))
            log.warn (sprint("temperature is {0} degrees!", 101));
        ---

        You may also need to use one of the various Layout & Appender 
        implementations to support your exact rendering needs.
        
        tango.log closely follows both the API and the behaviour as documented 
        at the official Log4J site, where you'll find a good tutorial. Those 
        pages are hosted over 
        <A HREF="http://logging.apache.org/log4j/docs/documentation.html">here</A>.

*******************************************************************************/

public class Logger : ILevel
{
        /***********************************************************************

                Add a trace message. This is called 'debug' in Log4J but
                that is a  reserved word in the D language
                
        ***********************************************************************/

        abstract Logger trace (char[] msg);

        /***********************************************************************
                
                Add an info message

        ***********************************************************************/

        abstract Logger info (char[] msg);

        /***********************************************************************

                Add a warning message

        ***********************************************************************/

        abstract Logger warn (char[] msg);

        /***********************************************************************

                Add an error message

        ***********************************************************************/

        abstract Logger error (char[] msg);

        /***********************************************************************

                Add a fatal message

        ***********************************************************************/

        abstract Logger fatal (char[] msg);

        /***********************************************************************
        
                Append a message to this logger via its appender list.

        ***********************************************************************/

        abstract Logger append (Level level, char[] s);

        /***********************************************************************
        
                Append a message to this logger using a delegate to 
                provide the content. Does not invoke the delegate if
                the logger is not enabled for the specified level.

        ***********************************************************************/

        // abstract Logger append (Level level, char[] delegate() dg);

        /***********************************************************************
        
                Return the name of this Logger

        ***********************************************************************/

        abstract char[] getName ();

        /***********************************************************************

                Return the current level assigned to this logger

        ***********************************************************************/

        abstract Level getLevel ();

        /***********************************************************************

                Set the activity level of this logger. Levels control how
                much information is emitted during runtime, and relate to
                each other as follows:
                ---
                    Trace < Info < Warn < Error < Fatal < None
                ---
                That is, if the level is set to Error, only calls to the
                error() and fatal() methods will actually produce output:
                all others will be inhibited.

                Note that Log4J is a hierarchical environment, and each
                logger defaults to inheriting a level from its parent.


        ***********************************************************************/

        abstract Logger setLevel (Level level = Level.Trace);

        /***********************************************************************
        
                same as setLevel (Level), but with additional control over 
                whether the children are forced to accept the changed level
                or not. If 'force' is false, then children adopt the parent
                level only if they have their own level set to Level.None

        ***********************************************************************/

        abstract Logger setLevel (Level level, bool force);

        /***********************************************************************
        
                Is this logger enabled for the provided level?

        ***********************************************************************/

        abstract bool isEnabled (Level level);

        /***********************************************************************

                Return whether this logger uses additive appenders or not. 
                See setAdditive().

        ***********************************************************************/

        abstract bool isAdditive ();

        /***********************************************************************

                Specify whether or not this logger has additive behaviour.
                This is enabled by default, and causes a logger to invoke
                all appenders within its ancestry (until an ancestor is
                found with an additive attribute of false).

        ***********************************************************************/

        abstract Logger setAdditive (bool enabled);

        /***********************************************************************

                Add an appender to this logger. You may add multiple
                appenders to appropriate loggers, and each of them 
                will be invoked for that given logger, and for each
                of its child loggers (assuming isAdditive() is true
                for those children). Note that multiple instances
                of the same appender, regardless of where they may
                reside within the tree, are not invoked at runtime.
                That is, only one from a set of identical loggers 
                will execute.

                Use clearAppenders() to remove all from a given logger.
                        
        ***********************************************************************/

        abstract Logger addAppender (Appender appender);

        /***********************************************************************
        
                Remove all appenders from this logger.

        ***********************************************************************/

        abstract Logger clearAppenders ();

        /***********************************************************************
        
                Get number of milliseconds since this application started

        ***********************************************************************/

        abstract ulong getRuntime ();
}
