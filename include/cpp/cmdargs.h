/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CMDARGS_H_
#define CMDARGS_H_

#include <stddef.h>
#include <stdarg.h>
#include <exception>
#include <vector>

namespace std {
	/**
	 * A usage-example:
	 *
	 * string s;
	 * s32 d = 0;
	 * u32 k = 0,flag = 0;
	 * cmdargs a(argc,argv,0);
	 * try {
	 *   a.parse("=s a1=d a2=k flag",&s,&a1,&a2,&flag);
	 *   if(a.is_help())
	 *     usage(argv[0]);
	 * }
	 * catch(const cmdargs_error& e) {
	 *   cerr << "Invalid arguments: " << e.what() << endl;
	 *   usage(argv[0]);
	 * }
	 */

	/**
	 * The exception that is thrown if an error occurred (= invalid arguments)
	 */
	class cmdargs_error : public exception {
	public:
		explicit cmdargs_error(const string& arg);
		virtual ~cmdargs_error() throw ();
		virtual const char* what() const throw ();
	private:
		string _msg;
	};

	/**
	 * The argument-parser
	 */
	class cmdargs {
	public:
		typedef unsigned char flag_type;

	public:
		/**
		 * Disallow '-' or '--' for arguments
		 */
		static const flag_type NO_DASHES	= 1;
		/**
		 * Require '=' for arguments
		 */
		static const flag_type REQ_EQ		= 2;
		/**
		 * Disallow '=' for arguments
		 */
		static const flag_type NO_EQ		= 4;
		/**
		 * Throw exception if free arguments are found
		 */
		static const flag_type NO_FREE		= 8;
		/**
		 * Max. 1 free argument
		 */
		static const flag_type MAX1_FREE	= 16;

	public:
		/**
		 * Creates a cmdargs-object for given arguments.
		 *
		 * @param argc the number of args
		 * @param argv the arguments
		 * @param flags the flags
		 */
		cmdargs(int argc,char * const *argv,flag_type flags = 0)
			: _argc(argc), _argv(argv), _flags(flags), _ishelp(false),
			  _args(vector<string*>()), _free(vector<string*>()) {
		}
		/**
		 * Destructor
		 */
		~cmdargs();

		/**
		 * Parses the command-line-arguments according to given format. It sets the given variable
		 * parameters depending on the format. The format works like the following:
		 * The first chars are the name of the argument, after that follows optionally a '=' which
		 * tells the parser that the argument has a value. Behind it the type of the value follows.
		 * The type may be:
		 * 	's':			a string, expects a std::string* as argument
		 * 	'd' or 'i':		a signed integer, expects a s32* as argument
		 * 	'u':			a unsigned integer to base 10, expects a u32* as argument
		 * 	'x' or 'X':		a unsigned integer to base 16, expects a u32* as argument
		 * 	'c':			a character, expects a char* as argument
		 * 	'k':			a unsigned integer with optional suffix K,M or G. expects a u32* as arg
		 * After the type may follow a '*' to tell the parser that the argument is required. If the
		 * argument has no value a bool* is expected as argument. The arguments have to be separated
		 * by spaces.
		 *
		 * An example is:
		 * bool flag;
		 * string s;
		 * s32 d;
		 * args->parse(args,"flag a=s arg2=d*",&flag,&s,&d);
		 *
		 * There are some special things to know:
		 * 	- If an required argument is missing, a argument-value is missing, free arguments are
		 *    given but not allowed, or similar, a cmdargs_error is thrown.
		 * 	- The name of an argument may be empty. This means implicit that its an required
		 *    argument which has just a value. Those arguments are expected in the given order. So
		 *    if you specify an empty argument as the first, it has to be the first in the
		 *    program-arguments (after the program-name of course).
		 * Additionally you can manipulate the behaviour by flags when creating the instance.
		 *
		 * @param fmt the format
		 * @throws cmdargs_error if parsing goes wrong
		 */
		void parse(const char *fmt,...);

		/**
		 * @return wether its a help-request (--help, -h, -?)
		 */
		bool is_help() const {
			return _ishelp;
		};

		/**
		 * @return a vector with all arguments (including the progname)
		 */
		const vector<string*> &get_args() const {
			return _args;
		};

		/**
		 * @return a vector with all free arguments
		 */
		const vector<string*> &get_free() const {
			return _free;
		};

	private:
		// no cloning
		cmdargs(const cmdargs& c);
		cmdargs& operator =(const cmdargs& c);

		/**
		 * Searches for the argument with given name in the args. Returns the argument-value
		 * and sets <pos> appropriatly to the argument-position in _args.
		 *
		 * @param name the argument-name
		 * @param hasVal wether the arg has a value
		 * @param pos will be set to the position in _args
		 * @return the value
		 */
		string find(const string& name,bool hasVal,vector<string*>::iterator& pos);
		/**
		 * Sets the value to the location pointed to by <ptr> in the format specified by <type>.
		 *
		 * @param arg the argument-value
		 * @param pos the position in _args
		 * @param hasVal wether the arg has a value
		 * @param type the arg-type
		 * @param ptr the location where to write to
		 */
		void setval(const string& arg,vector<string*>::iterator pos,bool hasVal,
				char type,void *ptr);
		/**
		 * Converts the given argument to an integer and treats 'K','M' and 'G' as binary-prefixes.
		 * I.e. K = 1024, M = 1024 * 1024, G = 1024 * 1024 * 1024
		 *
		 * @param arg the arg-value
		 * @return the integer
		 */
		unsigned int readk(const string& arg);

	private:
		int _argc;
		char * const *_argv;
		flag_type _flags;
		bool _ishelp;
		vector<string*> _args;
		vector<string*> _free;
	};
}

#endif /* CMDARGS_H_ */
