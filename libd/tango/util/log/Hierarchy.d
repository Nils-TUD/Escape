/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: October 2004
                        Delegate support proposed by BCS: August 2006 
        
        author:         Kris

*******************************************************************************/

module tango.util.log.Hierarchy;

private import  tango.util.log.Logger,
                tango.util.log.Appender;

private import  tango.util.log.model.IHierarchy;

/*******************************************************************************

        Pull in additional functions from the C library

*******************************************************************************/

extern (C)
{
        int memcmp (void *, void *, int);
}

/*******************************************************************************

        This is the real Logger implementation, hidden behind the public
        abstract frontage. 

*******************************************************************************/

private class LoggerInstance : Logger
{
        private LoggerInstance  next,
                                parent;

        private char[]          name;
        private Level           level;
        private Appender        appender;
        private Hierarchy       hierarchy;
        private bool            additive,
                                breakpoint;

        /***********************************************************************
        
                Construct a LoggerInstance with the specified name for the 
                given hierarchy. By default, logger instances are additive
                and are set to emit all events.

        ***********************************************************************/

        protected this (Hierarchy hierarchy, char[] name)
        {
                this.hierarchy = hierarchy;
                this.level = Level.Trace;
                this.additive = true;
                this.name = name;
        }

        /***********************************************************************
        
                Is this logger enabed for the specified Level?

        ***********************************************************************/

        final bool isEnabled (Level level)
        {
                return level >= this.level;
        }

        /***********************************************************************
        
                Is this a breakpoint Logger?
                
        ***********************************************************************/

        final bool isBreakpoint ()
        {
                return breakpoint;
        }

        /***********************************************************************
        
                Is this logger additive? That is, should we walk ancestors
                looking for more appenders?

        ***********************************************************************/

        final bool isAdditive ()
        {
                return additive;
        }

        /***********************************************************************

                Append a trace message

        ***********************************************************************/

        final Logger trace (char[] msg)
        {
                return append (Level.Trace, msg);
        }

        /***********************************************************************

                Append an info message

        ***********************************************************************/

        final Logger info (char[] msg)
        {
                return append (Level.Info, msg);
        }

        /***********************************************************************

                Append a warning message

        ***********************************************************************/

        final Logger warn (char[] msg)
        {
                return append (Level.Warn, msg);
        }

        /***********************************************************************

                Append an error message

        ***********************************************************************/

        final Logger error (char[] msg)
        {
                return append (Level.Error, msg);
        }

        /***********************************************************************

                Append a fatal message

        ***********************************************************************/

        final Logger fatal (char[] msg)
        {
                return append (Level.Fatal, msg);
        }

        /***********************************************************************

                Return the name of this Logger (sans the appended dot).
       
        ***********************************************************************/

        final char[] getName ()
        {
                int i = name.length;
                if (i > 0)
                    --i;
                return name[0 .. i];     
        }

        /***********************************************************************
        
                Return the Level this logger is set to

        ***********************************************************************/

        final Level getLevel ()
        {
                return level;     
        }

        /***********************************************************************
        
                Set the current level for this logger (and only this logger).

        ***********************************************************************/

        final Logger setLevel (Level level = Level.Trace)
        {
                return setLevel (level, false);
        }

        /***********************************************************************
        
                Set the current level for this logger, and (optionally) all
                of its descendents.

        ***********************************************************************/

        final Logger setLevel (Level level, bool force)
        {
                this.level = level;     
                hierarchy.updateLoggers (this, force);
                return this;
        }

        /***********************************************************************
        
                Set the breakpoint status of this logger.

        ***********************************************************************/

        final Logger setBreakpoint (bool enabled)
        {
                breakpoint = enabled;     
                hierarchy.updateLoggers (this, false);
                return this;
        }

        /***********************************************************************
        
                Set the additive status of this logger. See isAdditive().

        ***********************************************************************/

        final Logger setAdditive (bool enabled)
        {
                additive = enabled;     
                return this;
        }

        /***********************************************************************
        
                Add (another) appender to this logger. Appenders are each
                invoked for log events as they are produced. At most, one
                instance of each appender will be invoked.

        ***********************************************************************/

        final Logger addAppender (Appender next)
        {
                if (appender)
                    next.setNext (appender);
                appender = next;
                return this;
        }

        /***********************************************************************
        
                Remove all appenders from this Logger

        ***********************************************************************/

        final Logger clearAppenders ()
        {
                appender = null;     
                return this;
        }


        /***********************************************************************
        
                Get number of milliseconds since this application started

        ***********************************************************************/

        final ulong getRuntime ()
        {
                return Event.getRuntime;
        }

        /***********************************************************************
        
                Append a message to this logger using a delegate to 
                provide the content. Does not invoke the delegate if
                the the logger is not enabled for the specified level.

        ***********************************************************************/
/+
        final Logger append (Level level, char[] delegate() dg)
        {
                if (isEnabled (level))
                    append (level, dg());
                return this;
        }
+/
        /***********************************************************************
        
                Append a message to this logger via its appender list.

        ***********************************************************************/

        final Logger append (Level level, char[] s)
        {
                if (isEnabled (level))
                   {
                   auto event = Event.allocate();
                   scope (exit)
                          Event.deallocate (event);

                   // set the event attributes
                   event.set (hierarchy, level, s, name.length ? name[0..$-1] : "root");

                   // combine appenders from all ancestors
                   auto links = this;
                   Appender.Mask masks = 0;                 
                   do {
                      auto appender = links.appender;

                      // this level have an appender?
                      while (appender)
                            { 
                            auto mask = appender.getMask ();

                            // have we used this appender already?
                            if ((masks & mask) is 0)
                               {
                               // no - append message and update mask
                               event.scratch.length = 0;
                               appender.append (event);
                               masks |= mask;
                               }
                            // process all appenders for this node
                            appender = appender.getNext;
                            }
                        // process all ancestors
                      } while (links.additive && ((links = links.parent) !is null));
                   }
                return this;
        }

        /***********************************************************************
        
                See if the provided Logger is a good match as a parent of
                this one. Note that each Logger name has a '.' appended to
                the end, such that name segments will not partially match.

        ***********************************************************************/

        private final bool isCloserAncestor (LoggerInstance other)
        {
                auto length = other.name.length;

                // possible parent if length is shorter
                if (length < name.length)
                    // does the prefix match? Note we append a "." to each 
                    if (length is 0 || 
                        memcmp (&other.name[0], &name[0], length) is 0)
                        // is this a better (longer) match than prior parent?
                        if ((parent is null) || (length >= parent.name.length))
                             return true;
                return false;
        }
}


/*******************************************************************************
 
        The Logger hierarchy implementation. We keep a reference to each
        logger in a hash-table for convenient lookup purposes, plus keep
        each logger linked to the others in an ordered chain. Ordering
        places shortest names at the head and longest ones at the tail, 
        making the job of identifying ancestors easier in an orderly
        fashion. For example, when propogating levels across descendents
        it would be a mistake to propogate to a child before all of its
        ancestors were taken care of.

*******************************************************************************/

class Hierarchy : IHierarchy
{
        private char[]                  name,
                                        address;      

        private LoggerInstance          root;
        private LoggerInstance[char[]]  loggers;

        /***********************************************************************
        
                Construct a hierarchy with the given name.

        ***********************************************************************/

        this (char[] name)
        {
                this.name = name;
                this.address = "network";

                // insert a root node; the root has an empty name
                root = new LoggerInstance (this, "");
        }

        /**********************************************************************

                Return the name of this Hierarchy

        **********************************************************************/

        char[] getName ()
        {
                return name;
        }

        /**********************************************************************

                Return the address of this Hierarchy. This is typically
                attached when sending events to remote monitors.

        **********************************************************************/

        char[] getAddress ()
        {
                return address;
        }

        /**********************************************************************

                Set the name of this Hierarchy

        **********************************************************************/

        void setName (char[] name)
        {
                this.name = name;
        }

        /**********************************************************************

                Set the address of this Hierarchy. The address is attached
                used when sending events to remote monitors.

        **********************************************************************/

        void setAddress (char[] address)
        {
                this.address = address;
        }

        /***********************************************************************
        
                Return the root node.

        ***********************************************************************/

        LoggerInstance getRootLogger ()
        {
                return root;
        }

        /***********************************************************************
        
                Return the instance of a Logger with the provided name. If
                the instance does not exist, it is created at this time.

        ***********************************************************************/

        synchronized LoggerInstance getLogger (char[] name)
        {
                name ~= ".";

                LoggerInstance *l = name in loggers;

                if (l is null)
                   {
                   // create a new logger
                   LoggerInstance li = new LoggerInstance (this, name);
                   l = &li;

                   // insert into linked list
                   insertLogger (li);

                   // look for and adjust children
                   updateLoggers (li, true);

                   // insert into map
                   loggers [name] = li;
                   }
               
                return *l;
        }

        /**********************************************************************

                Iterate over all Loggers in list

        **********************************************************************/

        int opApply (int delegate(inout Logger) dg)
        {
                int result = 0;
                LoggerInstance curr = root;

                while (curr)
                      {
                      // BUG: this uncovers a cast() issue in the 'inout' delegation
                      Logger logger = curr;
                      if ((result = dg (logger)) != 0)
                           break;
                      curr = curr.next;
                      }
                return result;
        }

        /***********************************************************************
        
                Loggers are maintained in a sorted linked-list. The order 
                is maintained such that the shortest name is at the root, 
                and the longest at the tail.

                This is done so that updateLoggers() will always have a
                known environment to manipulate, making it much faster.

        ***********************************************************************/

        private void insertLogger (LoggerInstance l)
        {
                LoggerInstance prev,
                               curr = root;

                while (curr)
                      {
                      // insert here if the new name is shorter
                      if (l.name.length < curr.name.length)
                          if (prev is null)
                              throw new Exception ("invalid hierarchy");
                          else                                 
                             {
                             l.next = prev.next;
                             prev.next = l;
                             return;
                             }
                      else
                         // find best match for parent of new entry
                         propogate (l, curr, true);

                      // remember where insertion point should be
                      prev = curr;  
                      curr = curr.next;  
                      }

                // add to tail
                prev.next = l;
        }

        /***********************************************************************
        
                Propogate hierarchical changes across known loggers. 
                This includes changes in the hierarchy itself, and to
                the various settings of child loggers with respect to 
                their parent(s).              

        ***********************************************************************/

        private void updateLoggers (LoggerInstance changed, bool force)
        {
                LoggerInstance logger = root;

                // scan all loggers 
                while (logger)
                      {
                      propogate (logger, changed, force);

                      // try next entry
                      logger = logger.next;
                      }                
        }

        /***********************************************************************
        
                Propogate changes in the hierarchy downward to child Loggers.
                Note that while 'parent' and 'breakpoint' are always forced
                to update, the update of 'level' is selectable.

        ***********************************************************************/

        private void propogate (LoggerInstance logger, LoggerInstance changed, bool force)
        {
                // is the changed instance a better match for our parent?
                if (logger.isCloserAncestor (changed))
                   {
                   // update parent (might actually be current parent)
                   logger.parent = changed;

                   // if we don't have an explicit level set, inherit it
                   if (logger.level is Logger.Level.None || force)
                       logger.level = changed.level;

                   // always force breakpoints to follow parent settings
                   logger.breakpoint = changed.breakpoint;
                   }
        }
}

