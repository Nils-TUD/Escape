/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Split from Configurator.d, November 2005
        
        author:         Kris

*******************************************************************************/

module tango.util.log.PropertyConfigurator;

private import  tango.text.Properties;

private import  tango.io.FilePath;

private import  tango.util.log.Log,
                tango.util.log.Layout,
                tango.util.log.DateLayout,
                tango.util.log.Configurator,
                tango.util.log.ConsoleAppender;

private import  tango.util.log.model.ILevel;

/*******************************************************************************

        A utility class for initializing the basic behaviour of the
        default logging hierarchy.

        PropertyConfigurator parses a much simplified version of the 
        property file. tango.log only supports the settings of Logger 
        levels at this time; setup of Appenders and Layouts are currently 
        done "in the code"

*******************************************************************************/

struct PropertyConfigurator
{
        private static ILevel.Level[char[]] map;

        /***********************************************************************
        
                Populate a map of acceptable level names

        ***********************************************************************/

        static this()
        {
                map["TRACE"]    = ILevel.Level.Trace;
                map["trace"]    = ILevel.Level.Trace;
                map["INFO"]     = ILevel.Level.Info;
                map["info"]     = ILevel.Level.Info;
                map["WARN"]     = ILevel.Level.Warn;
                map["warn"]     = ILevel.Level.Warn;
                map["ERROR"]    = ILevel.Level.Error;
                map["error"]    = ILevel.Level.Error;
                map["FATAL"]    = ILevel.Level.Fatal;
                map["fatal"]    = ILevel.Level.Fatal;
                map["NONE"]     = ILevel.Level.None;
                map["none"]     = ILevel.Level.None;
        }

        /***********************************************************************
        
                Add a default StdioAppender, with a SimpleTimerLayout, to 
                the root node. The activity levels of all nodes are set
                via a property file with name=value pairs specified in the
                following format:

                    name: the actual logger name, in dot notation
                          format. The name "root" is reserved to
                          match the root logger node.

                   value: one of TRACE, INFO, WARN, ERROR, FATAL
                          or NONE (or the lowercase equivalents).

                For example, the declaration

                ---
                tango.unittest = INFO
                myApp.SocketActivity = TRACE
                ---
                
                sets the level of the loggers called tango.unittest and
                myApp.SocketActivity

        ***********************************************************************/

        static void configure (FilePath path)
        {
                void loader (char[] name, char[] value)
                {
                        auto l = (name == "root") ? Log.getRootLogger
                                                  : Log.getLogger (name);

                        if (l && (value in map))
                            l.setLevel (map[value]);
                }

                // setup the basic stuff
                Configurator.defaultAppender;

                // read and parse properties from file
                Properties!(char).load (path, &loader);
        }
}

