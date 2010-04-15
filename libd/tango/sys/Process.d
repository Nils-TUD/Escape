/*******************************************************************************
  copyright:   Copyright (c) 2006 Juan Jose Comellas. All rights reserved
  license:     BSD style: $(LICENSE)
  author:      Juan Jose Comellas <juanjo@comellas.com.ar>
*******************************************************************************/

module tango.sys.Process;

private import tango.io.model.IFile;
private import tango.io.Console;
private import tango.sys.Common;
import tango.sys.Environment;
private import tango.sys.Pipe;
private import tango.core.Exception;
private import tango.text.Util;
private import Integer = tango.text.convert.Integer;

private import tango.stdc.stdlib;
private import tango.stdc.string;
private import tango.stdc.stringz;

version (Escape)
{
    private import tango.stdc.errno;
	private import tango.stdc.io;
	private import tango.stdc.proc;
	private import tango.stdc.env;
	private import tango.stdc.stdio;
}
else version (Posix)
{
    private import tango.stdc.errno;
    private import tango.stdc.posix.fcntl;
    private import tango.stdc.posix.unistd;
    private import tango.stdc.posix.sys.wait;

    version (darwin)
    {
        extern (C) char*** _NSGetEnviron();
        private char** environ;
        
        static this ()
        {
            environ = *_NSGetEnviron();
        }
    }
    
    else
        private extern (C) extern char** environ;
}
else version (Windows)
{
  version (Win32SansUnicode)
  {
  }
  else
  {
    private import tango.text.convert.Utf : toString16;
  }
}

debug (Process)
{
    private import tango.io.Stdout;
}


/**
 * Redirect flags for processes.  Defined outside process class to cut down on
 * verbosity.
 */
enum Redirect
{
    /**
     * Redirect none of the standard handles
     */
    None = 0,

    /**
     * Redirect the stdout handle to a pipe.
     */
    Output = 1,

    /**
     * Redirect the stderr handle to a pipe.
     */
    Error = 2,

    /**
     * Redirect the stdin handle to a pipe.
     */
    Input = 4,

    /**
     * Redirect all three handles to pipes (default).
     */
    All = Output | Error | Input,

    /**
     * Send stderr to stdout's handle.  Note that the stderr PipeConduit will
     * be null.
     */
    ErrorToOutput = 0x10,

    /**
     * Send stdout to stderr's handle.  Note that the stdout PipeConduit will
     * be null.
     */
    OutputToError = 0x20,
}

/**
 * The Process class is used to start external programs and communicate with
 * them via their standard input, output and error streams.
 *
 * You can pass either the command line or an array of arguments to execute,
 * either in the constructor or to the args property. The environment
 * variables can be set in a similar way using the env property and you can
 * set the program's working directory via the workDir property.
 *
 * To actually start a process you need to use the execute() method. Once the
 * program is running you will be able to write to its standard input via the
 * stdin OutputStream and you will be able to read from its standard output and
 * error through the stdout and stderr InputStream respectively.
 *
 * You can check whether the process is running or not with the isRunning()
 * method and you can get its process ID via the pid property.
 *
 * After you are done with the process, or if you just want to wait for it to
 * end, you need to call the wait() method which will return once the process
 * is no longer running.
 *
 * To stop a running process you must use kill() method. If you do this you
 * cannot call the wait() method. Once the kill() method returns the process
 * will be already dead.
 * 
 * After calling either wait() or kill(), and no more data is expected on the
 * pipes, you should call close() as this will clean the pipes. Not doing this
 * may lead to a depletion of the available file descriptors for the main
 * process if many processes are created.
 *
 * Examples:
 * ---
 * try
 * {
 *     auto p = new Process ("ls -al", null);
 *     p.execute;
 *
 *     Stdout.formatln ("Output from {}:", p.programName);
 *     Stdout.copy (p.stdout).flush;
 *     auto result = p.wait;
 *
 *     Stdout.formatln ("Process '{}' ({}) exited with reason {}, status {}",
 *                      p.programName, p.pid, cast(int) result.reason, result.status);
 * }
 * catch (ProcessException e)
 *        Stdout.formatln ("Process execution failed: {}", e);
 * ---
 */
class Process
{
    /**
     * Result returned by wait().
     */
    public struct Result
    {
        /**
         * Reasons returned by wait() indicating why the process is no
         * longer running.
         */
        public enum
        {
            Exit,
            Signal,
            Stop,
            Continue,
            Error
        }

        public int reason;
        public int status;

        /**
         * Returns a string with a description of the process execution result.
         */
        public char[] toString()
        {
            char[] str;

            switch (reason)
            {
                case Exit:
                    str = format("Process exited normally with return code ", status);
                    break;

                case Signal:
                    str = format("Process was killed with signal ", status);
                    break;

                case Stop:
                    str = format("Process was stopped with signal ", status);
                    break;

                case Continue:
                    str = format("Process was resumed with signal ", status);
                    break;

                case Error:
                    str = format("Process failed with error code ", reason) ~
                                 " : " ~ SysError.lookup(status);
                    break;

                default:
                    str = format("Unknown process result ", reason);
                    break;
            }
            return str;
        }
    }

    static const uint DefaultStdinBufferSize    = 512;
    static const uint DefaultStdoutBufferSize   = 8192;
    static const uint DefaultStderrBufferSize   = 512;
    static const Redirect DefaultRedirectFlags  = Redirect.All;

    private char[][]        _args;
    private char[][char[]]  _env;
    private char[]          _workDir;
    private PipeConduit     _stdin;
    private PipeConduit     _stdout;
    private PipeConduit     _stderr;
    private bool            _running = false;
    private bool            _copyEnv = false;
    private Redirect        _redirect = DefaultRedirectFlags;

    version (Windows)
    {
        private PROCESS_INFORMATION *_info = null;
        private bool                 _gui = false;
    }
    else
    {
        private pid_t _pid = cast(pid_t) -1;
    }

    /**
     * Constructor (variadic version).  Note that by default, the environment
     * will not be copied.
     *
     * Params:
     * args     = array of strings with the process' arguments.  If there is
     *            exactly one argument, it is considered to contain the entire
     *            command line including parameters.  If you pass only one
     *            argument, spaces that are not intended to separate
     *            parameters should be embedded in quotes.  The arguments can
     *            also be empty.
     *
     * Examples:
     * ---
     * auto p = new Process("myprogram", "first argument", "second", "third");
     * auto p = new Process("myprogram \"first argument\" second third");
     * ---
     */
    public this(char[][] args ...)
    {
        if(args.length == 1)
            _args = splitArgs(args[0]);
        else
            _args = args;
    }

    /**
     * Constructor (variadic version, with environment copy).
     *
     * Params:
     * copyEnv  = if true, the environment is copied from the current process.
     * args     = array of strings with the process' arguments.  If there is
     *            exactly one argument, it is considered to contain the entire
     *            command line including parameters.  If you pass only one
     *            argument, spaces that are not intended to separate
     *            parameters should be embedded in quotes.  The arguments can
     *            also be empty.
     *
     * Examples:
     * ---
     * auto p = new Process(true, "myprogram", "first argument", "second", "third");
     * auto p = new Process(true, "myprogram \"first argument\" second third");
     * ---
     */
    public this(bool copyEnv, char[][] args ...)
    {
        _copyEnv = copyEnv;
        this(args);
    }

    /**
     * Constructor.
     *
     * Params:
     * command  = string with the process' command line; arguments that have
     *            embedded whitespace must be enclosed in inside double-quotes (").
     * env      = associative array of strings with the process' environment
     *            variables; the variable name must be the key of each entry.
     *
     * Examples:
     * ---
     * char[] command = "myprogram \"first argument\" second third";
     * char[][char[]] env;
     *
     * // Environment variables
     * env["MYVAR1"] = "first";
     * env["MYVAR2"] = "second";
     *
     * auto p = new Process(command, env)
     * ---
     */
    public this(char[] command, char[][char[]] env)
    in
    {
        assert(command.length > 0);
    }
    body
    {
        _args = splitArgs(command);
        _env = env;
    }

    /**
     * Constructor.
     *
     * Params:
     * args     = array of strings with the process' arguments; the first
     *            argument must be the process' name; the arguments can be
     *            empty.
     * env      = associative array of strings with the process' environment
     *            variables; the variable name must be the key of each entry.
     *
     * Examples:
     * ---
     * char[][] args;
     * char[][char[]] env;
     *
     * // Process name
     * args ~= "myprogram";
     * // Process arguments
     * args ~= "first argument";
     * args ~= "second";
     * args ~= "third";
     *
     * // Environment variables
     * env["MYVAR1"] = "first";
     * env["MYVAR2"] = "second";
     *
     * auto p = new Process(args, env)
     * ---
     */
    public this(char[][] args, char[][char[]] env)
    in
    {
        assert(args.length > 0);
        assert(args[0].length > 0);
    }
    body
    {
        _args = args;
        _env = env;
    }

    /**
     * Indicate whether the process is running or not.
     */
    public bool isRunning()
    {
        return _running;
    }

    /**
     * Return the running process' ID.
     *
     * Returns: an int with the process ID if the process is running;
     *          -1 if not.
     */
    public int pid()
    {
        version (Windows)
        {
            return (_info !is null ? cast(int) _info.dwProcessId : -1);
        }
        else // version (Posix)
        {
            return cast(int) _pid;
        }
    }

    /**
     * Return the process' executable filename.
     */
    public char[] programName()
    {
        return (_args !is null ? _args[0] : null);
    }

    /**
     * Set the process' executable filename.
     */
    public char[] programName(char[] name)
    {
        if (_args.length == 0)
        {
            _args.length = 1;
        }
        return _args[0] = name;
    }

    /**
     * Set the process' executable filename, return 'this' for chaining
     */
    public Process setProgramName(char[] name)
    {
        programName = name;
        return this;
    }

    /**
     * Return an array with the process' arguments.
     */
    public char[][] args()
    {
        return _args;
    }

    /**
     * Set the process' arguments from the arguments received by the method.
     *
     * Remarks:
     * The first element of the array must be the name of the process'
     * executable.
     *
     * Returns: the arugments that were set.
     *
     * Examples:
     * ---
     * p.args("myprogram", "first", "second argument", "third");
     * ---
     */
    public char[][] args(char[] progname, char[][] args ...)
    {
        return _args = progname ~ args;
    }

    /**
     * Set the process' arguments from the arguments received by the method.
     *
     * Remarks:
     * The first element of the array must be the name of the process'
     * executable.
     *
     * Returns: a reference to this for chaining
     *
     * Examples:
     * ---
     * p.setArgs("myprogram", "first", "second argument", "third").execute();
     * ---
     */
    public Process setArgs(char[] progname, char[][] args ...)
    {
        this.args(progname, args);
        return this;
    }

    /**
     * If true, the environment from the current process will be copied to the
     * child process.
     */
    public bool copyEnv()
    {
        return _copyEnv;
    }

    /**
     * Set the copyEnv flag.  If set to true, then the environment will be
     * copied from the current process.  If set to false, then the environment
     * is set from the env field.
     */
    public bool copyEnv(bool b)
    {
        return _copyEnv = b;
    }

    /**
     * Set the copyEnv flag.  If set to true, then the environment will be
     * copied from the current process.  If set to false, then the environment
     * is set from the env field.
     *
     * Returns:
     *   A reference to this for chaining
     */
    public Process setCopyEnv(bool b)
    {
        _copyEnv = b;
        return this;
    }

    /**
     * Return an associative array with the process' environment variables.
     *
     * Note that if copyEnv is set to true, this value is ignored.
     */
    public char[][char[]] env()
    {
        return _env;
    }

    /**
     * Set the process' environment variables from the associative array
     * received by the method.
     *
     * This also clears the copyEnv flag.
     *
     * Params:
     * env  = associative array of strings containing the environment
     *        variables for the process. The variable name should be the key
     *        used for each entry.
     *
     * Returns: the env set.
     * Examples:
     * ---
     * char[][char[]] env;
     *
     * env["MYVAR1"] = "first";
     * env["MYVAR2"] = "second";
     *
     * p.env = env;
     * ---
     */
    public char[][char[]] env(char[][char[]] env)
    {
        _copyEnv = false;
        return _env = env;
    }

    /**
     * Set the process' environment variables from the associative array
     * received by the method.  Returns a 'this' reference for chaining.
     *
     * This also clears the copyEnv flag.
     *
     * Params:
     * env  = associative array of strings containing the environment
     *        variables for the process. The variable name should be the key
     *        used for each entry.
     *
     * Returns: A reference to this process object
     * Examples:
     * ---
     * char[][char[]] env;
     *
     * env["MYVAR1"] = "first";
     * env["MYVAR2"] = "second";
     *
     * p.setEnv(env).execute();
     * ---
     */
    public Process setEnv(char[][char[]] env)
    {
        _copyEnv = false;
        _env = env;
        return this;
    }

    /**
     * Return an UTF-8 string with the process' command line.
     */
    public char[] toString()
    {
        char[] command;

        for (uint i = 0; i < _args.length; ++i)
        {
            if (i > 0)
            {
                command ~= ' ';
            }
            if (contains(_args[i], ' ') || _args[i].length == 0)
            {
                command ~= '"';
                command ~= _args[i].substitute("\\", "\\\\").substitute(`"`, `\"`);
                command ~= '"';
            }
            else
            {
                command ~= _args[i].substitute("\\", "\\\\").substitute(`"`, `\"`);
            }
        }
        return command;
    }

    /**
     * Return the working directory for the process.
     *
     * Returns: a string with the working directory; null if the working
     *          directory is the current directory.
     */
    public char[] workDir()
    {
        return _workDir;
    }

    /**
     * Set the working directory for the process.
     *
     * Params:
     * dir  = a string with the working directory; null if the working
     *         directory is the current directory.
     *
     * Returns: the directory set.
     */
    public char[] workDir(char[] dir)
    {
        return _workDir = dir;
    }

    /**
     * Set the working directory for the process.  Returns a 'this' reference
     * for chaining
     *
     * Params:
     * dir  = a string with the working directory; null if the working
     *         directory is the current directory.
     *
     * Returns: a reference to this process.
     */
    public Process setWorkDir(char[] dir)
    {
        _workDir = dir;
        return this;
    }

    /**
     * Get the redirect flags for the process.
     *
     * The redirect flags are used to determine whether stdout, stderr, or
     * stdin are redirected.  The flags are an or'd combination of which
     * standard handles to redirect.  A redirected handle creates a pipe,
     * whereas a non-redirected handle simply points to the same handle this
     * process is pointing to.
     *
     * You can also redirect stdout or stderr to each other.  The flags to
     * redirect a handle to a pipe and to redirect it to another handle are
     * mutually exclusive.  In the case both are specified, the redirect to
     * the other handle takes precedent.  It is illegal to specify both
     * redirection from stdout to stderr and from stderr to stdout.  If both
     * of these are specified, an exception is thrown.
     * 
     * If redirected to a pipe, once the process is executed successfully, its
     * input and output can be manipulated through the stdin, stdout and
     * stderr member PipeConduit's.  Note that if you redirect for example
     * stderr to stdout, and you redirect stdout to a pipe, only stdout will
     * be non-null.
     */
    public Redirect redirect()
    {
        return _redirect;
    }

    /**
     * Set the redirect flags for the process.
     */
    public Redirect redirect(Redirect flags)
    {
        return _redirect = flags;
    }

    /**
     * Set the redirect flags for the process.  Return a reference to this
     * process for chaining.
     */
    public Process setRedirect(Redirect flags)
    {
        _redirect = flags;
        return this;
    }

    /**
     * Get the GUI flag.
     *
     * This flag indicates on Windows systems that the CREATE_NO_WINDOW flag
     * should be set on CreateProcess.  Although this is a specific windows
     * flag, it is present on posix systems as a noop for compatibility.
     *
     * Without this flag, a console window will be allocated if it doesn't
     * already exist.
     */
    public bool gui()
    {
        version(Windows)
            return _gui;
        else
            return false;
    }

    /**
     * Set the GUI flag.
     *
     * This flag indicates on Windows systems that the CREATE_NO_WINDOW flag
     * should be set on CreateProcess.  Although this is a specific windows
     * flag, it is present on posix systems as a noop for compatibility.
     *
     * Without this flag, a console window will be allocated if it doesn't
     * already exist.
     */
    public bool gui(bool value)
    {
        version(Windows)
            return _gui = value;
        else
            return false;
    }

    /**
     * Set the GUI flag.  Returns a reference to this process for chaining.
     *
     * This flag indicates on Windows systems that the CREATE_NO_WINDOW flag
     * should be set on CreateProcess.  Although this is a specific windows
     * flag, it is present on posix systems as a noop for compatibility.
     *
     * Without this flag, a console window will be allocated if it doesn't
     * already exist.
     */
    public Process setGui(bool value)
    {
        version(Windows)
        {
            _gui = value;
        }
        return this;
    }

    /**
     * Return the running process' standard input pipe.
     *
     * Returns: a write-only PipeConduit connected to the child
     *          process' stdin.
     *
     * Remarks:
     * The stream will be null if no child process has been executed, or the
     * standard input stream was not redirected.
     */
    public PipeConduit stdin()
    {
        return _stdin;
    }

    /**
     * Return the running process' standard output pipe.
     *
     * Returns: a read-only PipeConduit connected to the child
     *          process' stdout.
     *
     * Remarks:
     * The stream will be null if no child process has been executed, or the
     * standard output stream was not redirected.
     */
    public PipeConduit stdout()
    {
        return _stdout;
    }

    /**
     * Return the running process' standard error pipe.
     *
     * Returns: a read-only PipeConduit connected to the child
     *          process' stderr.
     *
     * Remarks:
     * The stream will be null if no child process has been executed, or the
     * standard error stream was not redirected.
     */
    public PipeConduit stderr()
    {
        return _stderr;
    }

    /**
     * Execute a process using the arguments as parameters to this method.
     *
     * Once the process is executed successfully, its input and output can be
     * manipulated through the stdin, stdout and
     * stderr member PipeConduit's.
     *
     * Throws:
     * ProcessCreateException if the process could not be created
     * successfully; ProcessForkException if the call to the fork()
     * system call failed (on POSIX-compatible platforms).
     *
     * Remarks:
     * The process must not be running and the provided list of arguments must
     * not be empty. If there was any argument already present in the args
     * member, they will be replaced by the arguments supplied to the method.
     *
     * Deprecated: Use constructor or properties to set up process for
     * execution.
     */
    deprecated public void execute(char[] arg1, char[][] args ...)
    in
    {
        assert(!_running);
    }
    body
    {
        this._args = arg1 ~ args;
        execute();
    }

    /**
     * Execute a process using the command line arguments as parameters to
     * this method.
     *
     * Once the process is executed successfully, its input and output can be
     * manipulated through the stdin, stdout and
     * stderr member PipeConduit's.
     *
     * This also clears the copyEnv flag
     *
     * Params:
     * command  = string with the process' command line; arguments that have
     *            embedded whitespace must be enclosed in inside double-quotes (").
     * env      = associative array of strings with the process' environment
     *            variables; the variable name must be the key of each entry.
     *
     * Throws:
     * ProcessCreateException if the process could not be created
     * successfully; ProcessForkException if the call to the fork()
     * system call failed (on POSIX-compatible platforms).
     *
     * Remarks:
     * The process must not be running and the provided list of arguments must
     * not be empty. If there was any argument already present in the args
     * member, they will be replaced by the arguments supplied to the method.
     *
     * Deprecated: use properties or the constructor to set these parameters
     * instead.
     */
    deprecated public void execute(char[] command, char[][char[]] env)
    in
    {
        assert(!_running);
        assert(command.length > 0);
    }
    body
    {
        _args = splitArgs(command);
        _copyEnv = false;
        _env = env;
        execute();
    }

    /**
     * Execute a process using the command line arguments as parameters to
     * this method.
     *
     * Once the process is executed successfully, its input and output can be
     * manipulated through the stdin, stdout and
     * stderr member PipeConduit's.
     *
     * This also clears the copyEnv flag
     *
     * Params:
     * args     = array of strings with the process' arguments; the first
     *            argument must be the process' name; the arguments can be
     *            empty.
     * env      = associative array of strings with the process' environment
     *            variables; the variable name must be the key of each entry.
     *
     * Throws:
     * ProcessCreateException if the process could not be created
     * successfully; ProcessForkException if the call to the fork()
     * system call failed (on POSIX-compatible platforms).
     *
     * Remarks:
     * The process must not be running and the provided list of arguments must
     * not be empty. If there was any argument already present in the args
     * member, they will be replaced by the arguments supplied to the method.
     *
     * Deprecated:
     * Use properties or the constructor to set these parameters instead.
     *
     * Examples:
     * ---
     * auto p = new Process();
     * char[][] args;
     *
     * args ~= "ls";
     * args ~= "-l";
     *
     * p.execute(args, null);
     * ---
     */
    deprecated public void execute(char[][] args, char[][char[]] env)
    in
    {
        assert(!_running);
        assert(args.length > 0);
    }
    body
    {
        _args = args;
        _env = env;
        _copyEnv = false;

        execute();
    }

    /**
     * Execute a process using the arguments that were supplied to the
     * constructor or to the args property.
     *
     * Once the process is executed successfully, its input and output can be
     * manipulated through the stdin, stdout and
     * stderr member PipeConduit's.
     *
     * Returns:
     * A reference to this process object for chaining.
     *
     * Throws:
     * ProcessCreateException if the process could not be created
     * successfully; ProcessForkException if the call to the fork()
     * system call failed (on POSIX-compatible platforms).
     *
     * Remarks:
     * The process must not be running and the list of arguments must
     * not be empty before calling this method.
     */
    public Process execute()
    in
    {
        assert(!_running);
        assert(_args.length > 0 && _args[0] !is null);
    }
    body
    {
        version (Windows)
        {
            SECURITY_ATTRIBUTES sa;
            STARTUPINFO         startup;

            // We close and delete the pipes that could have been left open
            // from a previous execution.
            cleanPipes();

            // Set up the security attributes struct.
            sa.nLength = SECURITY_ATTRIBUTES.sizeof;
            sa.lpSecurityDescriptor = null;
            sa.bInheritHandle = true;

            // Set up members of the STARTUPINFO structure.
            memset(&startup, '\0', STARTUPINFO.sizeof);
            startup.cb = STARTUPINFO.sizeof;

            Pipe pin, pout, perr;
            if(_redirect != Redirect.None)
            {
                if((_redirect & (Redirect.OutputToError | Redirect.ErrorToOutput)) == (Redirect.OutputToError | Redirect.ErrorToOutput))
                    throw new ProcessCreateException(_args[0], "Illegal redirection flags", __FILE__, __LINE__);
                //
                // some redirection is specified, set the flag that indicates
                startup.dwFlags |= STARTF_USESTDHANDLES;

                // Create the pipes used to communicate with the child process.
                if(_redirect & Redirect.Input)
                {
                    pin = new Pipe(DefaultStdinBufferSize, &sa);
                    // Replace stdin with the "read" pipe
                    _stdin = pin.sink;
                    startup.hStdInput = cast(HANDLE) pin.source.fileHandle();
                    // Ensure the write handle to the pipe for STDIN is not inherited.
                    SetHandleInformation(cast(HANDLE) pin.sink.fileHandle(), HANDLE_FLAG_INHERIT, 0);
                }
                else
                {
                    // need to get the local process stdin handle
                    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
                }

                if((_redirect & (Redirect.Output | Redirect.OutputToError)) == Redirect.Output)
                {
                    pout = new Pipe(DefaultStdoutBufferSize, &sa);
                    // Replace stdout with the "write" pipe
                    _stdout = pout.source;
                    startup.hStdOutput = cast(HANDLE) pout.sink.fileHandle();
                    // Ensure the read handle to the pipe for STDOUT is not inherited.
                    SetHandleInformation(cast(HANDLE) pout.source.fileHandle(), HANDLE_FLAG_INHERIT, 0);
                }
                else
                {
                    // need to get the local process stdout handle
                    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
                }

                if((_redirect & (Redirect.Error | Redirect.ErrorToOutput)) == Redirect.Error)
                {
                    perr = new Pipe(DefaultStderrBufferSize, &sa);
                    // Replace stderr with the "write" pipe
                    _stderr = perr.source;
                    startup.hStdError = cast(HANDLE) perr.sink.fileHandle();
                    // Ensure the read handle to the pipe for STDOUT is not inherited.
                    SetHandleInformation(cast(HANDLE) perr.source.fileHandle(), HANDLE_FLAG_INHERIT, 0);
                }
                else
                {
                    // need to get the local process stderr handle
                    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
                }

                // do redirection from one handle to another
                if(_redirect & Redirect.ErrorToOutput)
                {
                    startup.hStdError = startup.hStdOutput;
                }

                if(_redirect & Redirect.OutputToError)
                {
                    startup.hStdOutput = startup.hStdError;
                }
            }
            
            // close the unused end of the pipes on scope exit
            scope(exit)
            {
                if(pin !is null)
                    pin.source.close();
                if(pout !is null)
                    pout.sink.close();
                if(perr !is null)
                    perr.sink.close();
            }

            _info = new PROCESS_INFORMATION;
            // Set up members of the PROCESS_INFORMATION structure.
            memset(_info, '\0', PROCESS_INFORMATION.sizeof);

           /* 
            * quotes and backslashes in the command line are handled very
            * strangely by Windows.  Through trial and error, I believe that
            * these are the rules:
            *
            * inside or outside quote mode:
            * 1. if 2 or more backslashes are followed by a quote, the first
            *    2 backslashes are reduced to 1 backslash which does not
            *    affect anything after it.
            * 2. one backslash followed by a quote is interpreted as a
            *    literal quote, which cannot be used to close quote mode, and
            *    does not affect anything after it.
            *
            * outside quote mode:
            * 3. a quote enters quote mode
            * 4. whitespace delineates an argument
            *
            * inside quote mode:
            * 5. 2 quotes sequentially are interpreted as a literal quote and
            *    an exit from quote mode.
            * 6. a quote at the end of the string, or one that is followed by
            *    anything other than a quote exits quote mode, but does not
            *    affect the character after the quote.
            * 7. end of line exits quote mode
            *
            * In our 'reverse' routine, we will only utilize the first 2 rules
            * for escapes.
            */
            char[] command;
            foreach(a; _args)
            {
              char[] nextarg = a.substitute(`"`, `\"`);
              //
              // find all instances where \\" occurs, and double all the
              // backslashes.  Otherwise, it will fall under rule 1, and those
              // backslashes will be halved.
              //
              uint pos = 0;
              while((pos = nextarg.locatePattern(`\\"`, pos)) < nextarg.length)
              {
                //
                // move back until we have all the backslashes
                //
                uint afterback = pos+1;
                while(pos > 0 && nextarg[pos - 1] == '\\')
                  pos--;

                //
                // double the number of backslashes that do not escape the
                // quote
                //
                nextarg = nextarg[0..afterback] ~ nextarg[pos..$];
                pos = afterback + afterback - pos + 2;
              }

              //
              // check to see if we need to surround the arg with quotes.
              //
              if(nextarg.length == 0)
              {
                nextarg = `""`;
              }
              else if(nextarg.contains(' '))
              {
                //
                // surround with quotes, but if the arg ends in backslashes,
                // we must double all the backslashes, or they will fall under
                // rule 1 and be halved.
                //
                
                if(nextarg[$-1] == '\\')
                {
                  //
                  // ends in a backslash.  count all the \'s at the end of the
                  // string, and repeat them
                  //
                  pos = nextarg.length - 1;
                  while(pos > 0 && nextarg[pos-1] == '\\')
                    pos--;
                  nextarg ~= nextarg[pos..$];
                }

                // surround the argument with quotes
                nextarg = '"' ~ nextarg ~ '"';
              }

              command ~= ' ';
              command ~= nextarg;
            }

            command ~= '\0';
            command = command[1..$];

            // old way
            //char[] command = toString();
            //command ~= '\0';

            version(Win32SansUnicode)
            {
              //
              // ASCII version of CreateProcess
              //

              // Convert the working directory to a null-ended string if
              // necessary.
              //
              // Note, this used to contain DETACHED_PROCESS, but
              // this causes problems with redirection if the program being
              // started decides to allocate a console (i.e. if you run a batch
              // file)
              if (CreateProcessA(null, command.ptr, null, null, true,
                    _gui ? CREATE_NO_WINDOW : 0,
                    (_copyEnv ? null : toNullEndedBuffer(_env).ptr),
                    toStringz(_workDir), &startup, _info))
              {
                CloseHandle(_info.hThread);
                _running = true;
              }
              else
              {
                throw new ProcessCreateException(_args[0], __FILE__, __LINE__);
              }
            }
            else
            {
              // Convert the working directory to a null-ended string if
              // necessary.
              //
              // Note, this used to contain DETACHED_PROCESS, but
              // this causes problems with redirection if the program being
              // started decides to allocate a console (i.e. if you run a batch
              // file)
              if (CreateProcessW(null, toString16(command).ptr, null, null, true,
                    _gui ? CREATE_NO_WINDOW : 0,
                    (_copyEnv ? null : toNullEndedBuffer(_env).ptr),
                    toString16z(toString16(_workDir)), &startup, _info))
              {
                CloseHandle(_info.hThread);
                _running = true;
              }
              else
              {
                throw new ProcessCreateException(_args[0], __FILE__, __LINE__);
              }
            }
        }
        else version (Escape)
        {
            // We close and delete the pipes that could have been left open
            // from a previous execution.
            cleanPipes();

            // validate the redirection flags
            if((_redirect & (Redirect.OutputToError | Redirect.ErrorToOutput)) == (Redirect.OutputToError | Redirect.ErrorToOutput))
                throw new ProcessCreateException(_args[0], "Illegal redirection flags", __FILE__, __LINE__);


            Pipe pin, pout, perr;
            if(_redirect & Redirect.Input)
                pin = new Pipe(DefaultStdinBufferSize);
            if((_redirect & (Redirect.Output | Redirect.OutputToError)) == Redirect.Output)
                pout = new Pipe(DefaultStdoutBufferSize);

            if((_redirect & (Redirect.Error | Redirect.ErrorToOutput)) == Redirect.Error)
                perr = new Pipe(DefaultStderrBufferSize);

            // This pipe is used to propagate the result of the call to
            // execv*() from the child process to the parent process.
            Pipe pexec = new Pipe(8);
            int status = 0;

            _pid = .fork();
            if (_pid >= 0)
            {
                if (_pid != 0)
                {
                    // Parent process
                    if(pin !is null)
                    {
                        _stdin = pin.sink;
                        pin.source.close();
                    }

                    if(pout !is null)
                    {
                        _stdout = pout.source;
                        pout.sink.close();
                    }

                    if(perr !is null)
                    {
                        _stderr = perr.source;
                        perr.sink.close();
                    }

                    pexec.sink.close();
                    scope(exit)
                       pexec.source.close();

                    try
                    {
                    	pexec.source.input.read((cast(byte*) &status)[0 .. status.sizeof]);
                    }
                    catch (Exception e)
                    {
                        // Everything's OK, the pipe was closed after the call to execv*()
                    }

                    if (status == 0)
                    {
                        _running = true;
                    }
                    else
                    {
                        // We set errno to the value that was sent through
                        // the pipe from the child process
                        errno = status;
                        _running = false;

                        throw new ProcessCreateException(_args[0], __FILE__, __LINE__);
                    }
                }
                else
                {
                    // Child process
                    int rc;
                    char*[] argptr;
                    char*[] envptr;

                    // Note that for all the pipes, we can close both ends
                    // because dup2 opens a duplicate file descriptor to the
                    // same resource.

                    // Replace stdin with the "read" pipe
                    if(pin !is null)
                    {
                        redirFd(STDIN_FILENO,pin.source.fileHandle());
                        pin.sink().close();
                        pin.source.close();
                    }

                    // Replace stdout with the "write" pipe
                    if(pout !is null)
                    {
                        redirFd(STDOUT_FILENO,pout.sink.fileHandle());
                        pout.source.close();
                        pout.sink.close();
                    }

                    // Replace stderr with the "write" pipe
                    if(perr !is null)
                    {
                        redirFd(STDERR_FILENO,perr.sink.fileHandle());
                        perr.source.close();
                        perr.sink.close();
                    }

                    // Check for redirection from stdout to stderr or vice
                    // versa
                    if(_redirect & Redirect.OutputToError)
                    {
                        redirFd(STDOUT_FILENO,STDERR_FILENO);
                    }

                    if(_redirect & Redirect.ErrorToOutput)
                    {
                        redirFd(STDERR_FILENO,STDOUT_FILENO);
                    }

                    // We close the unneeded part of the execv*() notification pipe
                    pexec.source.close();

                    // Convert the arguments and the environment variables to
                    // the format expected by the execv() family of functions.
                    argptr = toNullEndedArray(_args);
                	// TODO env is ignored here

                    // Switch to the working directory if it has been set.
                    if (_workDir.length > 0)
                    {
                    	.setEnv("CWD",toStringz(_workDir));
                    }

                    // Replace the child fork with a new process. We always use the
                    // system PATH to look for executables that don't specify
                    // directories in their names.
                    rc = execvpe(_args[0], argptr);
                    if (rc == -1)
                    {
                        Cerr("Failed to exec ")(_args[0])(": ")(SysError.lastMsg).newline;

                        try
                        {
                            status = errno;

                            // Propagate the child process' errno value to
                            // the parent process.
                            pexec.sink.output.write((cast(byte*) &status)[0 .. status.sizeof]);
                        }
                        catch (Exception e)
                        {
                        }
                        exit(errno);
                    }
                }
            }
            else
            {
                throw new ProcessForkException(_pid, __FILE__, __LINE__);
            }
        }
        else version (Posix)
        {
            // We close and delete the pipes that could have been left open
            // from a previous execution.
            cleanPipes();

            // validate the redirection flags
            if((_redirect & (Redirect.OutputToError | Redirect.ErrorToOutput)) == (Redirect.OutputToError | Redirect.ErrorToOutput))
                throw new ProcessCreateException(_args[0], "Illegal redirection flags", __FILE__, __LINE__);


            Pipe pin, pout, perr;
            if(_redirect & Redirect.Input)
                pin = new Pipe(DefaultStdinBufferSize);
            if((_redirect & (Redirect.Output | Redirect.OutputToError)) == Redirect.Output)
                pout = new Pipe(DefaultStdoutBufferSize);

            if((_redirect & (Redirect.Error | Redirect.ErrorToOutput)) == Redirect.Error)
                perr = new Pipe(DefaultStderrBufferSize);

            // This pipe is used to propagate the result of the call to
            // execv*() from the child process to the parent process.
            Pipe pexec = new Pipe(8);
            int status = 0;

            _pid = fork();
            if (_pid >= 0)
            {
                if (_pid != 0)
                {
                    // Parent process
                    if(pin !is null)
                    {
                        _stdin = pin.sink;
                        pin.source.close();
                    }

                    if(pout !is null)
                    {
                        _stdout = pout.source;
                        pout.sink.close();
                    }

                    if(perr !is null)
                    {
                        _stderr = perr.source;
                        perr.sink.close();
                    }

                    pexec.sink.close();
                    scope(exit)
                        pexec.source.close();

                    try
                    {
                        pexec.source.input.read((cast(byte*) &status)[0 .. status.sizeof]);
                    }
                    catch (Exception e)
                    {
                        // Everything's OK, the pipe was closed after the call to execv*()
                    }

                    if (status == 0)
                    {
                        _running = true;
                    }
                    else
                    {
                        // We set errno to the value that was sent through
                        // the pipe from the child process
                        errno = status;
                        _running = false;

                        throw new ProcessCreateException(_args[0], __FILE__, __LINE__);
                    }
                }
                else
                {
                    // Child process
                    int rc;
                    char*[] argptr;
                    char*[] envptr;

                    // Note that for all the pipes, we can close both ends
                    // because dup2 opens a duplicate file descriptor to the
                    // same resource.

                    // Replace stdin with the "read" pipe
                    if(pin !is null)
                    {
                        dup2(pin.source.fileHandle(), STDIN_FILENO);
                        pin.sink().close();
                        pin.source.close();
                    }

                    // Replace stdout with the "write" pipe
                    if(pout !is null)
                    {
                        dup2(pout.sink.fileHandle(), STDOUT_FILENO);
                        pout.source.close();
                        pout.sink.close();
                    }

                    // Replace stderr with the "write" pipe
                    if(perr !is null)
                    {
                        dup2(perr.sink.fileHandle(), STDERR_FILENO);
                        perr.source.close();
                        perr.sink.close();
                    }

                    // Check for redirection from stdout to stderr or vice
                    // versa
                    if(_redirect & Redirect.OutputToError)
                    {
                        dup2(STDERR_FILENO, STDOUT_FILENO);
                    }

                    if(_redirect & Redirect.ErrorToOutput)
                    {
                        dup2(STDOUT_FILENO, STDERR_FILENO);
                    }

                    // We close the unneeded part of the execv*() notification pipe
                    pexec.source.close();

                    // Set the "write" pipe so that it closes upon a successful
                    // call to execv*()
                    if (fcntl(cast(int) pexec.sink.fileHandle(), F_SETFD, FD_CLOEXEC) == 0)
                    {
                        // Convert the arguments and the environment variables to
                        // the format expected by the execv() family of functions.
                        argptr = toNullEndedArray(_args);
                        envptr = (_copyEnv ? null : toNullEndedArray(_env));

                        // Switch to the working directory if it has been set.
                        if (_workDir.length > 0)
                        {
                            chdir(toStringz(_workDir));
                        }

                        // Replace the child fork with a new process. We always use the
                        // system PATH to look for executables that don't specify
                        // directories in their names.
                        rc = execvpe(_args[0], argptr, envptr);
                        if (rc == -1)
                        {
                            Cerr("Failed to exec ")(_args[0])(": ")(SysError.lastMsg).newline;

                            try
                            {
                                status = errno;

                                // Propagate the child process' errno value to
                                // the parent process.
                                pexec.sink.output.write((cast(byte*) &status)[0 .. status.sizeof]);
                            }
                            catch (Exception e)
                            {
                            }
                            exit(errno);
                        }
                    }
                    else
                    {
                        Cerr("Failed to set notification pipe to close-on-exec for ")
                            (_args[0])(": ")(SysError.lastMsg).newline;
                        exit(errno);
                    }
                }
            }
            else
            {
                throw new ProcessForkException(_pid, __FILE__, __LINE__);
            }
        }
        else
        {
            assert(false, "tango.sys.Process: Unsupported platform");
        }
        return this;
    }


    /**
     * Unconditionally wait for a process to end and return the reason and
     * status code why the process ended.
     *
     * Returns:
     * The return value is a Result struct, which has two members:
     * reason and status. The reason can take the
     * following values:
     *
     * Process.Result.Exit: the child process exited normally;
     *                      status has the process' return
     *                      code.
     *
     * Process.Result.Signal: the child process was killed by a signal;
     *                        status has the signal number
     *                        that killed the process.
     *
     * Process.Result.Stop: the process was stopped; status
     *                      has the signal number that was used to stop
     *                      the process.
     *
     * Process.Result.Continue: the process had been previously stopped
     *                          and has now been restarted;
     *                          status has the signal number
     *                          that was used to continue the process.
     *
     * Process.Result.Error: We could not properly wait on the child
     *                       process; status has the
     *                       errno value if the process was
     *                       running and -1 if not.
     *
     * Remarks:
     * You can only call wait() on a running process once. The Signal, Stop
     * and Continue reasons will only be returned on POSIX-compatible
     * platforms.
     * Calling wait() will not clean the pipes as the parent process may still
     * want the remaining output. It is however recommended to call close()
     * when no more content is expected, as this will close the pipes.
     */
    public Result wait()
    {
        version (Windows)
        {
            Result result;

            if (_running)
            {
                DWORD rc;
                DWORD exitCode;

                assert(_info !is null);

                // We clean up the process related data and set the _running
                // flag to false once we're done waiting for the process to
                // finish.
                //
                // IMPORTANT: we don't delete the open pipes so that the parent
                //            process can get whatever the child process left on
                //            these pipes before dying.
                scope(exit)
                {
                    CloseHandle(_info.hProcess);
                    _running = false;
                }

                rc = WaitForSingleObject(_info.hProcess, INFINITE);
                if (rc == WAIT_OBJECT_0)
                {
                    GetExitCodeProcess(_info.hProcess, &exitCode);

                    result.reason = Result.Exit;
                    result.status = cast(typeof(result.status)) exitCode;

                    debug (Process)
                        Stdout.formatln("Child process '{0}' ({1}) returned with code {2}\n",
                                        _args[0], pid, result.status);
                }
                else if (rc == WAIT_FAILED)
                {
                    result.reason = Result.Error;
                    result.status = cast(short) GetLastError();

                    debug (Process)
                        Stdout.formatln("Child process '{0}' ({1}) failed "
                                        "with unknown exit status {2}\n",
                                        _args[0], pid, result.status);
                }
            }
            else
            {
                result.reason = Result.Error;
                result.status = -1;

                debug (Process)
                    Stdout.formatln("Child process '{0}' is not running", _args[0]);
            }
            return result;
        }
        else version (Escape)
        {
            Result result;

            if (_running)
            {
            	ExitState rc;

                // Wait for child process to end.
                if (waitChild(&rc) == 0 || rc.pid != _pid)
                {
                	if(rc.signal != SIG_COUNT)
                	{
                		result.reason = Result.Signal;
                		result.status = rc.signal;
                	}
                	else
                	{
                		result.reason = Result.Exit;
                		result.status = rc.exitCode;
                	}
                }
                else
                {
                    result.reason = Result.Error;
                    result.status = errno;

                    debug (Process)
	                    Stdout.formatln("Could not wait on child process '{0}' ({1}): ({2}) {3}",
	                            _args[0], _pid, result.status, SysError.lastMsg);
                }
            }
            else
            {
                result.reason = Result.Error;
                result.status = -1;

                debug (Process)
                    Stdout.formatln("Child process '{0}' is not running", _args[0]);
            }
            
            return result;
        }
        else version (Posix)
        {
            Result result;

            if (_running)
            {
                int rc;

                // We clean up the process related data and set the _running
                // flag to false once we're done waiting for the process to
                // finish.
                //
                // IMPORTANT: we don't delete the open pipes so that the parent
                //            process can get whatever the child process left on
                //            these pipes before dying.
                scope(exit)
                {
                    _running = false;
                }

                // Wait for child process to end.
                if (waitpid(_pid, &rc, 0) != -1)
                {
                    if (WIFEXITED(rc))
                    {
                        result.reason = Result.Exit;
                        result.status = WEXITSTATUS(rc);
                        if (result.status != 0)
                        {
                            debug (Process)
                                Stdout.formatln("Child process '{0}' ({1}) returned with code {2}\n",
                                                _args[0], _pid, result.status);
                        }
                    }
                    else
                    {
                        if (WIFSIGNALED(rc))
                        {
                            result.reason = Result.Signal;
                            result.status = WTERMSIG(rc);

                            debug (Process)
                                Stdout.formatln("Child process '{0}' ({1}) was killed prematurely "
                                                "with signal {2}",
                                                _args[0], _pid, result.status);
                        }
                        else if (WIFSTOPPED(rc))
                        {
                            result.reason = Result.Stop;
                            result.status = WSTOPSIG(rc);

                            debug (Process)
                                Stdout.formatln("Child process '{0}' ({1}) was stopped "
                                                "with signal {2}",
                                                _args[0], _pid, result.status);
                        }
                        else if (WIFCONTINUED(rc))
                        {
                            result.reason = Result.Stop;
                            result.status = WSTOPSIG(rc);

                            debug (Process)
                                Stdout.formatln("Child process '{0}' ({1}) was continued "
                                                "with signal {2}",
                                                _args[0], _pid, result.status);
                        }
                        else
                        {
                            result.reason = Result.Error;
                            result.status = rc;

                            debug (Process)
                                Stdout.formatln("Child process '{0}' ({1}) failed "
                                                "with unknown exit status {2}\n",
                                                _args[0], _pid, result.status);
                        }
                    }
                }
                else
                {
                    result.reason = Result.Error;
                    result.status = errno;

                    debug (Process)
                        Stdout.formatln("Could not wait on child process '{0}' ({1}): ({2}) {3}",
                                        _args[0], _pid, result.status, SysError.lastMsg);
                }
                clean();
            }
            else
            {
                result.reason = Result.Error;
                result.status = -1;

                debug (Process)
                    Stdout.formatln("Child process '{0}' is not running", _args[0]);
            }
            return result;
        }
        else
        {
            assert(false, "tango.sys.Process: Unsupported platform");
        }
    }

    /**
     * Kill a running process. This method will not return until the process
     * has been killed.
     *
     * Throws:
     * ProcessKillException if the process could not be killed;
     * ProcessWaitException if we could not wait on the process after
     * killing it.
     *
     * Remarks:
     * After calling this method you will not be able to call wait() on the
     * process.
     * Killing the process does not clean the attached pipes as the parent
     * process may still want/need the remaining content. However, it is
     * recommended to call close() on the process when it is no longer needed
     * as this will clean the pipes. 
     */
    public void kill()
    {
        version (Windows)
        {
            if (_running)
            {
                assert(_info !is null);

                if (TerminateProcess(_info.hProcess, cast(UINT) -1))
                {
                    assert(_info !is null);

                    // We clean up the process related data and set the _running
                    // flag to false once we're done waiting for the process to
                    // finish.
                    //
                    // IMPORTANT: we don't delete the open pipes so that the parent
                    //            process can get whatever the child process left on
                    //            these pipes before dying.
                    scope(exit)
                    {
                        CloseHandle(_info.hProcess);
                        _running = false;
                    }

                    // FIXME: We should probably use a timeout here
                    if (WaitForSingleObject(_info.hProcess, INFINITE) == WAIT_FAILED)
                    {
                        throw new ProcessWaitException(cast(int) _info.dwProcessId,
                                                       __FILE__, __LINE__);
                    }
                }
                else
                {
                    throw new ProcessKillException(cast(int) _info.dwProcessId,
                                                   __FILE__, __LINE__);
                }
            }
            else
            {
                debug (Process)
                    Stdout.print("Tried to kill an invalid process");
            }
        }
        else version (Escape)
        {
            if (_running)
            {
            	ExitState rc;

                assert(_pid > 0);

                if (.sendSignalTo(cast(pid_t)_pid, SIG_TERM, 0) == 0)
                {
                    // We clean up the process related data and set the _running
                    // flag to false once we're done waiting for the process to
                    // finish.
                    //
                    // IMPORTANT: we don't delete the open pipes so that the parent
                    //            process can get whatever the child process left on
                    //            these pipes before dying.
                    scope(exit)
					{
                        _running = false;
					}

                    // FIXME: is this loop really needed?
                    for (uint i = 0; i < 100; i++)
                    {
                        int res = waitChild(&rc);
                        if (rc.pid == _pid)
                        {
                            break;
                        }
                        else if (res < 0)
                        {
                            throw new ProcessWaitException(cast(int) _pid, __FILE__, __LINE__);
                        }
                        .sleep(50);
                    }
                }
                else
                {
                    throw new ProcessKillException(_pid, __FILE__, __LINE__);
                }
            }
            else
            {
                debug (Process)
                    Stdout.print("Tried to kill an invalid process");
            }
        }
        else version (Posix)
        {
            if (_running)
            {
                int rc;

                assert(_pid > 0);

                if (.kill(_pid, SIGTERM) != -1)
                {
                    // We clean up the process related data and set the _running
                    // flag to false once we're done waiting for the process to
                    // finish.
                    //
                    // IMPORTANT: we don't delete the open pipes so that the parent
                    //            process can get whatever the child process left on
                    //            these pipes before dying.
                    scope(exit)
                    {
                        _running = false;
                    }

                    // FIXME: is this loop really needed?
                    for (uint i = 0; i < 100; i++)
                    {
                        rc = waitpid(pid, null, WNOHANG | WUNTRACED);
                        if (rc == _pid)
                        {
                            break;
                        }
                        else if (rc == -1)
                        {
                            throw new ProcessWaitException(cast(int) _pid, __FILE__, __LINE__);
                        }
                        usleep(50000);
                    }
                }
                else
                {
                    throw new ProcessKillException(_pid, __FILE__, __LINE__);
                }
            }
            else
            {
                debug (Process)
                    Stdout.print("Tried to kill an invalid process");
            }
        }
        else
        {
            assert(false, "tango.sys.Process: Unsupported platform");
        }
    }

    /**
     * Split a string containing the command line used to invoke a program
     * and return and array with the parsed arguments. The double-quotes (")
     * character can be used to specify arguments with embedded spaces.
     * e.g. first "second param" third
     */
    protected static char[][] splitArgs(ref char[] command, char[] delims = " \t\r\n")
    in
    {
        assert(!contains(delims, '"'),
               "The argument delimiter string cannot contain a double quotes ('\"') character");
    }
    body
    {
        enum State
        {
            Start,
            FindDelimiter,
            InsideQuotes
        }

        char[][]    args = null;
        char[][]    chunks = null;
        int         start = -1;
        char        c;
        int         i;
        State       state = State.Start;

        // Append an argument to the 'args' array using the 'chunks' array
        // and the current position in the 'command' string as the source.
        void appendChunksAsArg()
        {
            uint argPos;

            if (chunks.length > 0)
            {
                // Create the array element corresponding to the argument by
                // appending the first chunk.
                args   ~= chunks[0];
                argPos  = args.length - 1;

                for (uint chunkPos = 1; chunkPos < chunks.length; ++chunkPos)
                {
                    args[argPos] ~= chunks[chunkPos];
                }

                if (start != -1)
                {
                    args[argPos] ~= command[start .. i];
                }
                chunks.length = 0;
            }
            else
            {
                if (start != -1)
                {
                    args ~= command[start .. i];
                }
            }
            start = -1;
        }

        for (i = 0; i < command.length; i++)
        {
            c = command[i];

            switch (state)
            {
                // Start looking for an argument.
                case State.Start:
                    if (c == '"')
                    {
                        state = State.InsideQuotes;
                    }
                    else if (!contains(delims, c))
                    {
                        start = i;
                        state = State.FindDelimiter;
                    }
                    else
                    {
                        appendChunksAsArg();
                    }
                    break;

                // Find the ending delimiter for an argument.
                case State.FindDelimiter:
                    if (c == '"')
                    {
                        // If we find a quotes character this means that we've
                        // found a quoted section of an argument. (e.g.
                        // abc"def"ghi). The quoted section will be appended
                        // to the preceding part of the argument. This is also
                        // what Unix shells do (i.e. a"b"c becomes abc).
                        if (start != -1)
                        {
                            chunks ~= command[start .. i];
                            start = -1;
                        }
                        state = State.InsideQuotes;
                    }
                    else if (contains(delims, c))
                    {
                        appendChunksAsArg();
                        state = State.Start;
                    }
                    break;

                // Inside a quoted argument or section of an argument.
                case State.InsideQuotes:
                    if (start == -1)
                    {
                        start = i;
                    }

                    if (c == '"')
                    {
                        chunks ~= command[start .. i];
                        start = -1;
                        state = State.Start;
                    }
                    break;

                default:
                    assert(false, "Invalid state in Process.splitArgs");
            }
        }

        // Add the last argument (if there is one)
        appendChunksAsArg();

        return args;
    }

    /**
     * Close and delete any pipe that may have been left open in a previous
     * execution of a child process.
     */
    protected void cleanPipes()
    {
        delete _stdin;
        delete _stdout;
        delete _stderr;
    }

    /**
     * Explicitly close any resources held by this process object. It is recommended
     * to always call this when you are done with the process.
     */
    public void close()
    {
        this.cleanPipes;
    }

    version (Windows)
    {
        /**
         * Convert an associative array of strings to a buffer containing a
         * concatenation of "<name>=<value>" strings separated by a null
         * character and with an additional null character at the end of it.
         * This is the format expected by the CreateProcess() Windows API for
         * the environment variables.
         */
        protected static char[] toNullEndedBuffer(char[][char[]] src)
        {
            char[] dest;

            foreach (key, value; src)
            {
                dest ~= key ~ '=' ~ value ~ '\0';
            }

            dest ~= "\0\0";
            return dest;
        }
    }
    else version (Escape)
    {
        /**
         * Convert an array of strings to an array of pointers to char with
         * a terminating null character (C strings). The resulting array
         * has a null pointer at the end. This is the format expected by
         * the execv*() family of POSIX functions.
         */
        protected static char*[] toNullEndedArray(char[][] src)
        {
            if (src !is null)
            {
                char*[] dest = new char*[src.length + 1];
                int     i = src.length;

                // Add terminating null pointer to the array
                dest[i] = null;

                while (--i >= 0)
                {
                    // Add a terminating null character to each string
                    dest[i] = toStringz(src[i]);
                }
                return dest;
            }
            else
            {
                return null;
            }
        }

        /**
         * Convert an associative array of strings to an array of pointers to
         * char with a terminating null character (C strings). The resulting
         * array has a null pointer at the end. This is the format expected by
         * the execv*() family of POSIX functions for environment variables.
         */
        protected static char*[] toNullEndedArray(char[][char[]] src)
        {
            char*[] dest;

            foreach (key, value; src)
            {
                dest ~= (key ~ '=' ~ value ~ '\0').ptr;
            }

            dest ~= null;
            return dest;
        }

        /**
         * Execute a process by looking up a file in the system path, passing
         * the array of arguments and the the environment variables. This
         * method is a combination of the execve() and execvp() POSIX system
         * calls.
         */
        protected static int execvpe(char[] filename, char*[] argv)
        in
        {
            assert(filename.length > 0);
        }
        body
        {
            int rc = -1;

            if (!contains(filename, FileConst.PathSeparatorChar))
            {
                char[] str = Environment.get("PATH");
                char[][] pathList = delimit(str, ":");

                foreach (path; pathList)
                {
                    if (path[path.length - 1] != FileConst.PathSeparatorChar)
                    {
                        path ~= FileConst.PathSeparatorChar;
                    }

                    debug (Process)
                        Stdout.formatln("Trying execution of '{0}' in directory '{1}'",
                                        filename, path);

                    path ~= filename;
                    path ~= '\0';

                    rc = exec(path.ptr, argv.ptr);
                    // If the process execution failed because of an error
                    // other than ENOENT (No such file or directory) we
                    // abort the loop.
                    // TODO
                    if (rc < 0/* && errno != ENOENT*/)
                    {
                        break;
                    }
                }
            }
            else
            {
                debug (Process)
                    Stdout.formatln("Calling execve('{0}', argv[{1}])",
                                    (argv[0])[0 .. strlen(argv[0])],argv.length);

                rc = exec(argv[0], argv.ptr);
            }
            return rc;
        }
    }
    else version (Posix)
    {
        /**
         * Convert an array of strings to an array of pointers to char with
         * a terminating null character (C strings). The resulting array
         * has a null pointer at the end. This is the format expected by
         * the execv*() family of POSIX functions.
         */
        protected static char*[] toNullEndedArray(char[][] src)
        {
            if (src !is null)
            {
                char*[] dest = new char*[src.length + 1];
                int     i = src.length;

                // Add terminating null pointer to the array
                dest[i] = null;

                while (--i >= 0)
                {
                    // Add a terminating null character to each string
                    dest[i] = toStringz(src[i]);
                }
                return dest;
            }
            else
            {
                return null;
            }
        }

        /**
         * Convert an associative array of strings to an array of pointers to
         * char with a terminating null character (C strings). The resulting
         * array has a null pointer at the end. This is the format expected by
         * the execv*() family of POSIX functions for environment variables.
         */
        protected static char*[] toNullEndedArray(char[][char[]] src)
        {
            char*[] dest;

            foreach (key, value; src)
            {
                dest ~= (key ~ '=' ~ value ~ '\0').ptr;
            }

            dest ~= null;
            return dest;
        }

        /**
         * Execute a process by looking up a file in the system path, passing
         * the array of arguments and the the environment variables. This
         * method is a combination of the execve() and execvp() POSIX system
         * calls.
         */
        protected static int execvpe(char[] filename, char*[] argv, char*[] envp)
        in
        {
            assert(filename.length > 0);
        }
        body
        {
            int rc = -1;
            char* str;

            if (!contains(filename, FileConst.PathSeparatorChar) &&
                (str = getenv("PATH")) !is null)
            {
                char[][] pathList = delimit(str[0 .. strlen(str)], ":");

                foreach (path; pathList)
                {
                    if (path[path.length - 1] != FileConst.PathSeparatorChar)
                    {
                        path ~= FileConst.PathSeparatorChar;
                    }

                    debug (Process)
                        Stdout.formatln("Trying execution of '{0}' in directory '{1}'",
                                        filename, path);

                    path ~= filename;
                    path ~= '\0';

                    rc = execve(path.ptr, argv.ptr, (envp.length == 0 ? environ : envp.ptr));
                    // If the process execution failed because of an error
                    // other than ENOENT (No such file or directory) we
                    // abort the loop.
                    if (rc == -1 && errno != ENOENT)
                    {
                        break;
                    }
                }
            }
            else
            {
                debug (Process)
                    Stdout.formatln("Calling execve('{0}', argv[{1}], {2})",
                                    (argv[0])[0 .. strlen(argv[0])],
                                    argv.length, (envp.length > 0 ? "envp" : "null"));

                rc = execve(argv[0], argv.ptr, (envp.length == 0 ? environ : envp.ptr));
            }
            return rc;
        }
    }
}


/**
 * Exception thrown when the process cannot be created.
 */
class ProcessCreateException: ProcessException
{
    public this(char[] command, char[] file, uint line)
    {
        this(command, SysError.lastMsg, file, line);
    }

    public this(char[] command, char[] message, char[] file, uint line)
    {
        super("Could not create process for " ~ command ~ " : " ~ message);
    }
}

/**
 * Exception thrown when the parent process cannot be forked.
 *
 * This exception will only be thrown on POSIX-compatible platforms.
 */
class ProcessForkException: ProcessException
{
    public this(int pid, char[] file, uint line)
    {
        super(format("Could not fork process ", pid) ~ " : " ~ SysError.lastMsg);
    }
}

/**
 * Exception thrown when the process cannot be killed.
 */
class ProcessKillException: ProcessException
{
    public this(int pid, char[] file, uint line)
    {
        super(format("Could not kill process ", pid) ~ " : " ~ SysError.lastMsg);
    }
}

/**
 * Exception thrown when the parent process tries to wait on the child
 * process and fails.
 */
class ProcessWaitException: ProcessException
{
    public this(int pid, char[] file, uint line)
    {
        super(format("Could not wait on process ", pid) ~ " : " ~ SysError.lastMsg);
    }
}




/**
 *  append an int argument to a message
*/
private char[] format (char[] msg, int value)
{
    char[10] tmp;

    return msg ~ Integer.format (tmp, value);
}


debug (UnitTest)
{
    private import tango.io.stream.Lines;

    unittest
    {
        char[][] params;
        char[] command = "echo ";

        params ~= "one";
        params ~= "two";
        params ~= "three";

        command ~= '"';
        foreach (i, param; params)
        {
            command ~= param;
            if (i != params.length - 1)
            {
                command ~= '\n';
            }
        }
        command ~= '"';

        try
        {
            auto p = new Process(command, null);

            p.execute();

            foreach (i, line; new Lines!(char)(p.stdout))
            {
                if (i == params.length) // echo can add ending new line confusing this test
                    break;
                assert(line == params[i]);
            }

            auto result = p.wait();

            assert(result.reason == Process.Result.Exit && result.status == 0);
        }
        catch (ProcessException e)
        {
            Cerr("Program execution failed: ")(e.toString()).newline();
        }
    }
}

