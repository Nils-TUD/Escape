/*******************************************************************************

        copyright:      Copyright (c) 2009 Kris. All rights reserved.

        license:        BSD style: $(LICENSE)
        
        version:        Oct 2009: Initial release
        
        author:         Kris
    
*******************************************************************************/

module tango.text.Arguments;

private import tango.text.Util;
private import tango.util.container.more.Stack;

debug private import tango.io.Stdout;

/*******************************************************************************

        Command-line argument parser. Simple usage is:
        ---
        auto args = new Arguments;
        args.parse ("-a -b", true);
        auto a = args("a");
        auto b = args("b");
        if (a.set && b.set)
            ...
        ---

        Argument parameters are assigned to the last known target, such
        that multiple parameters accumulate:
        ---
        args.parse ("-a=1 -a=2 foo", true);
        assert (args('a').assigned.length is 3);
        ---

        That example results in argument 'a' assigned three parameters.
        Two parameters are explicitly assigned using '=', while a third
        is implicitly assigned. Implicit parameters are often useful for
        collecting filenames or other parameters without specifying the
        associated argument:
        ---
        args.parse ("thisfile.txt thatfile.doc -v", true);
        assert (args(null).assigned.length is 2);
        ---
        The 'null' argument is always defined and acts as an accumulator
        for parameters left uncaptured by other arguments. In the above
        instance it was assigned both parameters. 
        
        Examples thus far have used 'sloppy' argument declaration, via
        the second argument of parse() being set true. This allows the
        parser to create argument declaration on-the-fly, which can be
        handy for trivial usage. However, most features require the a-
        priori declaration of arguments:
        ---
        args = new Arguments;
        args('x').required;
        if (! args.parse("-x"))
              // x not supplied!
        ---

        Sloppy arguments are disabled in that example, and a required
        argument 'x' is declared. The parse() method will fail if the
        pre-conditions are not fully met. Additional qualifiers include
        specifying how many parameters are allowed for each individual
        argument, default parameters, whether an argument requires the 
        presence or exclusion of another, etc. Qualifiers are typically 
        chained together and the following example shows argument "foo"
        being made required, with one parameter, aliased to 'f', and
        dependent upon the presence of another argument "bar":
        ---
        args("foo").required.params(1).aliased('f').requires("bar");
        args("help").aliased('?').aliased('h');
        ---

        Parameters can be constrained to a set of matching text values,
        and the parser will fail on mismatched input:
        ---
        args("greeting").restrict("hello", "yo", "gday");
        args("enabled").restrict("true", "false", "t", "f", "y", "n");  
        ---

        A set of declared arguments may be configured in this manner
        and the parser will return true only where all conditions are
        met. Where a error condition occurs you may traverse the set
        of arguments to find out which argument has what error. This
        can be handled like so, where arg.error holds a defined code:
        ---
        if (! args.parse (...))
              foreach (arg; args)
                       if (arg.error)
                           ...
        ---
       
        Error codes are as follows:
        ---
        None:           ok (zero)
        ParamLo:        too few params for an argument
        ParamHi:        too many params for an argument
        Required:       missing argument is required 
        Requires:       depends on a missing argument
        Conflict:       conflicting argument is present
        Extra:          unexpected argument (see sloppy)
        Option:         parameter does not match options
        ---
        
        A simpler way to handle errors is to invoke an internal format
        routine, which constructs error messages on your behalf:
        ---
        if (! args.parse (...))
              stderr (args.errors(&stderr.layout.sprint));
        ---

        Note that messages are constructed via a layout handler and
        the messages themselves may be customized (for i18n purposes).
        See the two errors() methods for more information on this.

        The parser make a distinction between a short and long prefix, 
        in that a long prefix argument is always distinct while short
        prefix arguments may be combined as a shortcut:
        ---
        args.parse ("--foo --bar -abc", true);
        assert (args("foo").set);
        assert (args("bar".set);
        assert (args("a").set);
        assert (args("b").set);
        assert (args("c").set);
        ---

        In addition, short-prefix arguments may be "smushed" with an
        associated parameter when configured to do so:
        ---
        args('o').params(1).smush;
        if (args.parse ("-ofile"))
            assert (args('o').assigned[0] == "file");
        ---

        There are two callback varieties supports, where one is invoked
        when an associated argument is parsed and the other is invoked
        as parameters are assigned. See the bind() methods for delegate
        signature details.

        You may change the argument prefix to be something other than 
        "-" and "--" via the constructor. You might, for example, need 
        to specify a "/" indicator instead, and use ':' for explicitly
        assigning parameters:
        ---
        auto args = new Args ("/", "-", ':');
        args.parse ("-foo:param -bar /abc");
        assert (args("foo").set);
        assert (args("bar".set);
        assert (args("a").set);
        assert (args("b").set);
        assert (args("c").set);
        assert (args("foo").assigned.length is 1);
        ---

        Returning to an earlier example we can declare some specifics:
        ---
        args('v').params(0);
        assert (args.parse (`-v thisfile.txt thatfile.doc`));
        assert (args(null).assigned.length is 2);
        ---

        Note that the -v flag is now in front of the implicit parameters
        but ignores them because it is declared to consume none. That is,
        implicit parameters are assigned to arguments from right to left,
        according to how many parameters said arguments may consume. Each
        sloppy argument consumes parameters by default, so those implicit
        parameters would have been assigned to -v without the declaration 
        shown. On the other hand, an explicit assignment (via '=') always 
        associates the parameter with that argument even when an overflow
        would occur (though will cause an error to be raised).

        Certain parameters are used for capturing comments or other plain
        text from the user, including whitespace and other special chars.
        Such parameter values should be quoted on the commandline, and be
        assigned explicitly rather than implicitly:
        ---
        args.parse (`--comment="-- a comment --"`);
        ---

        Without the explicit assignment, the text content might otherwise 
        be considered the start of another argument (due to how argv/argc
        values are stripped of original quotes).

        Lastly, all subsequent text is treated as paramter-values after a
        "--" token is encountered. This notion is applied by unix systems 
        to terminate argument processing in a similar manner. Such values
        are considered to be implicit, and are assigned to preceding args
        in the usual right to left fashion (or to the null argument):
        ---
        args.parse (`-- -thisfile --thatfile`);
        assert (args(null).assigned.length is 2);
        ---
        
*******************************************************************************/

class Arguments
{
        public alias get                opCall;         // args("name")
        public alias get                opIndex;        // args["name"]

        private Stack!(Argument)        stack;          // args with params
        private Argument[char[]]        args;           // the set of args
        private Argument[char[]]        aliases;        // set of aliases
        private char                    eq;             // '=' or ':'
        private char[]                  sp,             // short prefix
                                        lp;             // long prefix
        private char[][]                msgs = errmsg;  // error messages
        private const char[][]          errmsg =        // default errors
                [
                "argument '{0}' expects {2} parameter(s) but has {1}\n", 
                "argument '{0}' expects {3} parameter(s) but has {1}\n", 
                "argument '{0}' is missing\n", 
                "argument '{0}' requires '{4}'\n", 
                "argument '{0}' conflicts with '{4}'\n", 
                "unexpected argument '{0}'\n", 
                "argument '{0}' expects one of {5}\n", 
                ];

        /***********************************************************************
              
              Construct with the specific short & long prefixes, and the 
              given assignment character (typically ':' on Windows but we
              set the defaults to look like unix instead)

        ***********************************************************************/
        
        this (char[] sp="-", char[] lp="--", char eq='=')
        {
                this.sp = sp;
                this.lp = lp;
                this.eq = eq;
                get(null).params;       // set null argument to consume params
        }

        /***********************************************************************
              
                Parse string[] into a set of Argument instances. The 'sloppy'
                option allows for unexpected arguments without error.
                
                Returns false where an error condition occurred, whereupon the 
                arguments should be traversed to discover said condition(s):
                ---
                auto args = new Arguments;
                if (! args.parse (...))
                      stderr (args.errors(&stderr.layout.sprint));
                ---

        ***********************************************************************/
        
        final bool parse (char[] input, bool sloppy=false)
        {
                char[][] tmp;
                foreach (s; quotes(input, " "))
                         tmp ~= s;
                return parse (tmp, sloppy);
        }

        /***********************************************************************
              
                Parse a string into a set of Argument instances. The 'sloppy'
                option allows for unexpected arguments without error.
                
                Returns false where an error condition occurred, whereupon the 
                arguments should be traversed to discover said condition(s):
                ---
                auto args = new Arguments;
                if (! args.parse (...))
                      Stderr (args.errors(&Stderr.layout.sprint));
                ---

        ***********************************************************************/
        
        final bool parse (char[][] input, bool sloppy=false)
        {
                bool    done;
                int     error;

                debug stdout.formatln ("\ncmdline: '{}'", input);
                stack.push (get(null));
                foreach (s; input)
                         if (s.length)
                            {
                            debug stdout.formatln ("'{}'", s);
                            if (! done)
                                  if (s == "--")
                                     {done=true; continue;}
                                  else
                                     if (argument (s, lp, sloppy, false) ||
                                         argument (s, sp, sloppy, true))
                                         continue;
                            stack.top.append (s);
                            }  
                foreach (arg; args)
                         error |= arg.valid;
                return error is 0;
        }

        /***********************************************************************
              
                Clear parameter assignments, flags and errors. Note this 
                does not remove any Arguments

        ***********************************************************************/
        
        final Arguments clear ()
        {
                stack.clear;
                foreach (arg; args)
                        {
                        arg.set = false;
                        arg.values = null;
                        arg.error = arg.None;
                        }
                return this;
        }

        /***********************************************************************
              
                Obtain an argument reference, creating an new instance where
                necessary. Use array indexing or opCall syntax if you prefer

        ***********************************************************************/
        
        final Argument get (char name)
        {
                return get ((&name)[0..1]);
        }

        /***********************************************************************
              
                Obtain an argument reference, creating an new instance where
                necessary. Use array indexing or opCall syntax if you prefer

        ***********************************************************************/
        
        final Argument get (char[] name)
        {
                auto a = name in args;
                if (a is null)
                   {name=name.dup; return args[name] = new Argument(name);}
                return *a;
        }

        /***********************************************************************

                Traverse the set of arguments

        ***********************************************************************/

        final int opApply (int delegate(ref Argument) dg)
        {
                int result;
                foreach (arg; args)  
                         if ((result=dg(arg)) != 0)
                              break;
                return result;
        }

        /***********************************************************************

                Construct a string of error messages, using the given
                delegate to format the output. You would typically pass
                the system formatter here, like so:
                ---
                auto msgs = args.errors (&stderr.layout.sprint);
                ---

                The messages are replacable with custom (i18n) versions
                instead, using the errors(char[][]) method 

        ***********************************************************************/

        char[] errors (char[] delegate(char[] buf, char[] fmt, ...) dg)
        {
                char[256] tmp;
                char[] result;
                foreach (arg; args)
                         if (arg.error)
                             result ~= dg (tmp, msgs[arg.error-1], arg.name, 
                                           arg.values.length, arg.min, arg.max, 
                                           arg.bogus, arg.options);
                return result;                             
        }

        /***********************************************************************
                
                Use this method to replace the default error messages. Note
                that arguments are passed to the formatter in the following
                order, and these should be indexed appropriately by each of
                the error messages (see examples in errmsg above):
                ---
                index 0: the argument name
                index 1: number of parameters
                index 2: configured minimum parameters
                index 3: configured maximum parameters
                index 4: conflicting/dependent argument name
                index 5: array of configured parameter options
                ---

        ***********************************************************************/

        Arguments errors (char[][] errors)
        {
                if (errors.length is errmsg.length)
                    msgs = errors;
                return this;
        }

        /***********************************************************************
              
                Test for the presence of a switch (long/short prefix) 
                and enable the associated arg where found. Also look 
                for and handle explicit parameter assignment
                
        ***********************************************************************/
        
        private bool argument (char[] s, char[] p, bool sloppy, bool flag)
        {
                if (s.length >= p.length && s[0..p.length] == p)
                   {
                   s = s [p.length..$];
                   auto i = locate (s, eq);
                   if (i < s.length)
                      {
                      enable (s[0..i], sloppy, flag);
                      stack.top.append (s[i+1..$], true);
                      }
                   else
                      enable (s, sloppy, flag);
                   return true;
                   }
                return false;
        }

        /***********************************************************************
              
                Indicate the existance of an argument, and handle sloppy
                options along with multiple-flags and smushed parameters.
                Note that sloppy arguments are configured with parameters
                enabled.

        ***********************************************************************/
        
        private Argument enable (char[] elem, bool sloppy, bool flag=false)
        {
                if (flag && elem.length > 1)
                   {
                   // locate arg for first char
                   auto arg = enable (elem[0..1], sloppy);
                   elem = elem[1..$];

                   // smush the remaining text, or treat then as more args
                   if (arg.cat)
                       arg.append (elem, true);
                   else
                      foreach (c; elem)
                               arg = enable ((&c)[0..1], sloppy);
                   return arg;
                   }

                // if not in args, or in aliases, then create new arg
                auto a = elem in args;
                if (a is null)
                    if ((a = elem in aliases) is null)
                         return get(elem).params.enable(!sloppy);
                return a.enable;
        }

        /***********************************************************************
              
                A specific argument instance. You get one of these from 
                Arguments.get() and visit them via Arguments.opApply()

        ***********************************************************************/
        
        class Argument
        {       
                /***************************************************************
                
                        Error identifiers:
                        ---
                        None:           ok
                        ParamLo:        too few params for an argument
                        ParamHi:        too many params for an argument
                        Required:       missing argument is required 
                        Requires:       depends on a missing argument
                        Conflict:       conflicting argument is present
                        Extra:          unexpected argument (see sloppy)
                        Option:         parameter does not match options
                        ---

                ***************************************************************/
        
                enum {None, ParamLo, ParamHi, Required, Requires, Conflict, Extra, Option};

                alias void   delegate() Invoker;
                alias void   delegate(char[] value) Inspector;

                public int              min,            /// minimum params
                                        max,            /// maximum params
                                        error;          /// error condition
                public  bool            set;            /// arg is present
                private bool            req,            // arg is required
                                        cat,            // arg is smushable
                                        exp,            // implicit params
                                        fail;           // fail the parse
                private char[]          name,           // arg name
                                        bogus;          // name of conflict
                private char[][]        values,         // assigned values
                                        options,        // validation options
                                        deefalts;       // configured defaults
                private Invoker         invoker;        // invocation callback
                private Inspector       inspector;      // inspection callback
                private Argument[]      dependees,      // who we require
                                        conflictees;    // who we conflict with
                
                /***************************************************************
              
                        Create with the given name

                ***************************************************************/
        
                this (char[] name)
                {
                        this.name = name;
                }

                /***************************************************************
              
                        Return the name of this argument

                ***************************************************************/
        
                override char[] toString()
                {
                        return name;
                }

                /***************************************************************
                
                        return the assigned parameters, or the defaults if
                        no parameters were assigned

                ***************************************************************/
        
                final char[][] assigned ()
                {
                        return values.length ? values : deefalts;
                }

                /***************************************************************
              
                        Alias this argument with the given name. If you need 
                        long-names to be aliased, create the long-name first
                        and alias it to a short one

                ***************************************************************/
        
                final Argument aliased (char name)
                {
                        this.outer.aliases[(&name)[0..1].dup] = this;
                        return this;
                }

                /***************************************************************
              
                        Make this argument a requirement

                ***************************************************************/
        
                final Argument required ()
                {
                        this.req = true;
                        return this;
                }

                /***************************************************************
              
                        Set this argument to depend upon another

                ***************************************************************/
        
                final Argument requires (Argument arg)
                {
                        dependees ~= arg;
                        return this;
                }

                /***************************************************************
              
                        Set this argument to depend upon another

                ***************************************************************/
        
                final Argument requires (char[] other)
                {
                        return requires (this.outer.get(other));
                }

                /***************************************************************
              
                        Set this argument to depend upon another

                ***************************************************************/
        
                final Argument requires (char other)
                {
                        return requires ((&other)[0..1]);
                }

                /***************************************************************
              
                        Set this argument to conflict with another

                ***************************************************************/
        
                final Argument conflicts (Argument arg)
                {
                        conflictees ~= arg;
                        return this;
                }

                /***************************************************************
              
                        Set this argument to conflict with another

                ***************************************************************/
        
                final Argument conflicts (char[] other)
                {
                        return conflicts (this.outer.get(other));
                }

                /***************************************************************
              
                        Set this argument to conflict with another

                ***************************************************************/
        
                final Argument conflicts (char other)
                {
                        return conflicts ((&other)[0..1]);
                }

                /***************************************************************
              
                        Enable parameter assignment: 0 to 42 by default

                ***************************************************************/
        
                final Argument params ()
                {
                        return params (0, 42);
                }

                /***************************************************************
              
                        Set an exact number of parameters required

                ***************************************************************/
        
                final Argument params (int count)
                {
                        return params (count, count);
                }

                /***************************************************************
              
                        Set both the minimum and maximum parameter counts

                ***************************************************************/
        
                final Argument params (int min, int max)
                {
                        this.min = min;
                        this.max = max;
                        return this;
                }

                /***************************************************************
                        
                        Add another default parameter for this argument

                ***************************************************************/
        
                final Argument defaults (char[] values)
                {
                        this.deefalts ~= values;
                        return this;
                }

                /***************************************************************
              
                        Set an inspector for this argument, fired when a
                        parameter is appended to an argument

                ***************************************************************/
        
                final Argument bind (Inspector inspector)
                {
                        this.inspector = inspector;
                        return this;
                }

                /***************************************************************
              
                        Set an invoker for this argument, fired when an
                        argument declaration is seen

                ***************************************************************/
        
                final Argument bind (Invoker invoker)
                {
                        this.invoker = invoker;
                        return this;
                }

                /***************************************************************
              
                        Enable smushing for this argument, where "-ofile" 
                        would result in "file" being assigned to argument 
                        'o'

                ***************************************************************/
        
                final Argument smush ()
                {
                        cat = true;
                        return this;
                }

                /***************************************************************
              
                ***************************************************************/
        
                final Argument explicit ()
                {
                        exp = true;
                        return this;
                }

                /***************************************************************
              
                        Alter the title of this argument, which can be 
                        useful for naming the default argument

                ***************************************************************/
        
                final Argument title (char[] name)
                {
                        this.name = name;
                        return this;
                }

                /***************************************************************
              
                        Fail the parse when this arg is encountered. You
                        might use this for managing help text

                ***************************************************************/
        
                final Argument halt ()
                {
                        this.fail = true;
                        return this;
                }

                /***************************************************************
              
                        Restrict values to one of the given set

                ***************************************************************/
        
                final Argument restrict (char[][] options ...)
                {
                        this.options = options;
                        return this;
                }

                /***************************************************************
              
                        This arg is present, but set an error condition
                        (Extra) when unexpected and sloppy is not enabled.
                        Fires any configured invoker callback.

                ***************************************************************/
        
                private Argument enable (bool unexpected=false)
                {
                        this.set = true;
                        if (max > 0)
                            this.outer.stack.push(this);

                        if (invoker)
                            invoker();
                        if (unexpected)
                            error = Extra;
                        return this;
                }

                /***************************************************************
              
                        Append a parameter value, invoking an inspector as
                        necessary

                ***************************************************************/
        
                private void append (char[] value, bool explicit=false)
                {       
                        // pop to an argument that can accept implicit parameters?
                        if (explicit is false)
                            for (auto s=&this.outer.stack; exp && s.size>1; this=s.top)
                                 s.pop;

                        this.set = true;        // needed for default assignments 
                        values ~= value;        // append new value

                        if (inspector)
                            inspector (value);

                        if (options.length && !error)
                           {
                           error = Option;
                           foreach (option; options)
                                    if (option == value)
                                        error = None;
                           }

                        // pop to an argument that can accept parameters
                        for (auto s=&this.outer.stack; values.length >= max && s.size>1; this=s.top)
                             s.pop;
                }

                /***************************************************************
                
                        Test and set the error flag appropriately 

                ***************************************************************/
        
                private int valid ()
                {
                        if (error is None)
                            if (req && !set)      
                                error = Required;
                            else
                               if (set)
                                  {
                                  // short circuit?
                                  if (fail)
                                      return -1;

                                  if (values.length < min)
                                      error = ParamLo;
                                  else
                                     if (values.length > max)
                                         error = ParamHi;
                                     else
                                        {
                                        foreach (arg; dependees)
                                                 if (! arg.set)
                                                       error = Requires, bogus=arg.name;

                                        foreach (arg; conflictees)
                                                 if (arg.set)
                                                     error = Conflict, bogus=arg.name;
                                        }
                                  }

                        debug stdout.formatln ("{}: error={}, set={}, min={}, max={}, "
                                               "req={}, values={}, defaults={}, requires={}", 
                                               name, error, set, min, max, req, values, 
                                               deefalts, dependees);
                        return error;
                }
        }
}


/*******************************************************************************
      
*******************************************************************************/

debug(UnitTest)
{
        unittest
        {
        auto args = new Arguments;

        // basic 
        auto x = args['x'];
        assert (args.parse (""));
        x.required;
        assert (args.parse ("") is false);
        assert (args.clear.parse ("-x"));
        assert (x.set);

        // alias
        x.aliased('X');
        assert (args.clear.parse ("-X"));
        assert (x.set);

        // unexpected arg (with sloppy)
        assert (args.clear.parse ("-y") is false);
        assert (args.clear.parse ("-y") is false);
        assert (args.clear.parse ("-y", true) is false);
        assert (args['y'].set);
        assert (args.clear.parse ("-x -y", true));

        // parameters
        x.params(0);
        assert (args.clear.parse ("-x param"));
        assert (x.assigned.length is 0);
        assert (args(null).assigned.length is 1);
        x.params(1);
        assert (args.clear.parse ("-x=param"));
        assert (x.assigned.length is 1);
        assert (x.assigned[0] == "param");
        assert (args.clear.parse ("-x param"));
        assert (x.assigned.length is 1);
        assert (x.assigned[0] == "param");

        // too many args
        x.params(1);
        assert (args.clear.parse ("-x param1 param2"));
        assert (x.assigned.length is 1);
        assert (x.assigned[0] == "param1");
        assert (args(null).assigned.length is 1);
        assert (args(null).assigned[0] == "param2");
        

        // now with default params
        assert (args.clear.parse ("param1 param2 -x=blah"));
        assert (args[null].assigned.length is 2);
        assert (args(null).assigned.length is 2);
        assert (x.assigned.length is 1);
        x.params(0);
        assert (args.clear.parse ("-x=blah"));
        assert (args(null).assigned.length is 1);


        // multiple flags, with alias and sloppy
        assert (args.clear.parse ("-xy"));
        assert (args.clear.parse ("-xyX"));
        assert (x.set);
        assert (args['y'].set);
        assert (args.clear.parse ("-xyz") is false);
        assert (args.clear.parse ("-xyz", true));
        auto z = args['z'];
        assert (z.set);

        // multiple flags with trailing arg
        assert (args.clear.parse ("-xyz=10"));
        assert (z.assigned.length is 1);

        // again, but without sloppy param declaration
        z.params(0);
        assert (args.clear.parse ("-xyz=10"));
        assert (args('y').assigned.length is 1);
        assert (args('x').assigned.length is 0);
        assert (args('z').assigned.length is 0);

        // x requires y
        x.requires('y');
        assert (args.clear.parse ("-xy"));
        assert (args.clear.parse ("-xz") is false);

        // defaults
        z.defaults("foo");
        assert (args.clear.parse ("-xy"));
        assert (z.assigned.length is 1);

        // long names, with params
        assert (args.clear.parse ("-xy --foobar") is false);
        assert (args.clear.parse ("-xy --foobar", true));
        assert (args["y"].set && x.set);
        assert (args["foobar"].set);
        assert (args.clear.parse ("-xy --foobar=10"));
        assert (args["foobar"].assigned.length is 1);
        assert (args["foobar"].assigned[0] == "10");

        // smush argument z, but not others
        z.params;
        assert (args.clear.parse ("-xy -zsmush") is false);
        assert (x.set);
        z.smush;
        assert (args.clear.parse ("-xy -zsmush"));
        assert (z.assigned.length is 1);
        assert (z.assigned[0] == "smush");
        assert (x.assigned.length is 0);
        z.params(0);

        // conflict x with z
        x.conflicts(z);
        assert (args.clear.parse ("-xyz") is false);

        // word mode, with prefix elimination
        args = new Arguments (null, null);
        assert (args.clear.parse ("foo bar wumpus") is false);
        assert (args.clear.parse ("foo bar wumpus wombat", true));
        assert (args("foo").set);
        assert (args("bar").set);
        assert (args("wumpus").set);
        assert (args("wombat").set);

        // use '/' instead of '-'
        args = new Arguments ("/", "/");
        assert (args.clear.parse ("/foo /bar /wumpus") is false);
        assert (args.clear.parse ("/foo /bar /wumpus /wombat", true));
        assert (args("foo").set);
        assert (args("bar").set);
        assert (args("wumpus").set);
        assert (args("wombat").set);

        // use '/' for short and '-' for long
        args = new Arguments ("/", "-");
        assert (args.clear.parse ("-foo -bar -wumpus -wombat /abc", true));
        assert (args("foo").set);
        assert (args("bar").set);
        assert (args("wumpus").set);
        assert (args("wombat").set);
        assert (args("a").set);
        assert (args("b").set);
        assert (args("c").set);

        // "--" makes all subsequent be implicit parameters
        args = new Arguments;
        args('f').params(2);
        assert (args.parse ("-f -- -bar -wumpus -wombat --abc"));
        assert (args('f').assigned.length is 2);
        assert (args(null).assigned.length is 2);
        }
}

/*******************************************************************************
      
*******************************************************************************/

debug (Arguments)
{       
        import tango.io.Stdout;

        void main()
        {
                auto args = new Arguments;

                args(null).title("root").params;
                args('x').aliased('X').params(0).required;
                args('y').defaults("hi").params(2).smush.explicit;
                args('a').required.defaults("hi").requires('y').params(1);
                args("foobar").params(2);
                if (! args.parse ("'one =two' -ax=bar -y=ff -yss --foobar=blah1 --foobar barf blah2"))
                      stdout (args.errors(&stdout.layout.sprint));
        }
}