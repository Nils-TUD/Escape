#!/usr/bin/perl
################################################################################################################
# gSTLFilt.pl: BD Software STL Error Message Decryptor (a Perl script)
#			   This version supports the gcc 2/3/4 C++ compiler/library
#			   It was tested under:
#                         DJGPP 2.95.2
#                         MinGW gcc 2.95.2, 3.2.3, 3.4.5, 4.1.1
#                         TDM gcc 4.2.2
#


$STLFilt_ID = "BD Software STL Message Decryptor v3.10 for gcc 2/3/4";

#
# (c) Copyright Leor Zolman 2002-2008. Permission to copy, use, modify, sell and
# distribute this software is granted provided this copyright notice appears
# in all copies. This software is provided "as is" without express or implied
# warranty, and with no claim as to its suitability for any purpose.
#
#################################################################################################################
#              Visit www.bdsoft.com for information about BD Software's on-site training seminars               #
#                     in C, C++, STL, Perl, Unix Fundamentals and Korn Shell Programming                        #
#################################################################################################################
#
# For quick installation instructions for the STL Error Decryptor, see QUICKSTART.txt.
# For manifest and other general information, see README.txt
#
# (Note: hard tab setting for this source file is: 4)
#
# Purpose:  Transform STL template-based typenames from gcc error messages
#			into a minimal, *readable* format. We might lose some details...
#			but retain our sanity! 
#
# This script may be used in several different ways:
#	1) With the "Proxy C++" program, C++.EXE (from the command-line or from within Dev-CPP)
#	2) With a batch/shell script driver for command-line use...
#        For Windows: The companion batch file GFILT.BAT is provided as a sample driver for this script
#        For Unix/Linux/OS X: The companion shell script "gfilt" is provided as a sample driver for
#	        this script, allowing arbitrary intermixing of compiler and Decryptor options
#	        on the command line.
#
# Acknowledgements:
#   Scott Meyers taught his "Effective STL" seminar, where the project began.
#   Thomas Becker wrote and helps me maintain the Win32 piping code used throughout both C++.CPP and 
#       the Perl scripts, keeping everyone talking despite all the curves thrown him (so far) by
#       ActiveState and Microsoft. THANK YOU, Thomas!
#   David Abrahams designed the long-typename-wrapping algorithm and continues to contribute actively
#		to its evolution.
#   David Smallberg came up with the "Proxy compiler" idea (but blame me for the name).
#
# For the complete list of folks whose feedback and de-bugging help contributed to this
# package, and also for the list of programming courses BD Software offers, see README.txt.
#
#
#################################################################################################################
#
#   Script Options
#   --------------
#
#	Command line options are case insensitive, and may be preceded by either '-' or '/'.
#   The Proxy C++, if being used, supplies many of these options as per settings in the
#   Proxy-gcc.INI configuration file.
#
#	Note that some of the Decryptor's behavior is controlled via command-line options,
#	while other behavior may only be configured via hard-wired variable settings. Please
#   examine the entire "User-Configurable Settings" section below to become familiar with
#   all the customizable features.
#
#	General options:
#
#			-iter:x				Set iterator policy to x, where x is s[short], m[edium] or l[ong]
#								(See the assignments of $def_iter_policy and $newiter below for details)
#
#			-cand:x				Set "candidates" policy to x, where x is L[ong], M[edium]  or  S[hort]
#								(See the assignment of $def_cand_policy below for details)
#
#			-hdr:x				Set STL header policy to x or nn, where x is L[ong], M[edium], or S[hort],
#			-hdr:nn				or nn is the # of errors to show in a cluster
#			-hdr:LD				Dave Abrahams re-ordering mode 1 (see below)
#			-hdr:LD1			save as above
#			-hdr:LD2			Dave Abrahams re-ordering mode 2
#								(See the assignment of $def_hdr_policy below for details)
#
#			-with:x				Set "with clause" substitution policy to x, where x is L[ong] or  S[hort]
#								(See the assignment of $def_with_policy below for details)
#
#			-path:x				Set "show long pathnames" policy to x, where x is l[ong] or s[hort]
#								(See the assignment of $def_path_policy below for details)
#
#			-showback:x			Set backtrace policy: Y (default) or N (suppress all backtrace lines)
#
#			-width:nn			Set output line width (break message lines at this column, or 0 for NO wrapping)
#
#			-lognative			Log all native messages to NativeLog.txt (for de-bugging)
#
#			-banner:x			Show Decryptor banner line:  Y (default) or N
#
#			-nullarg:xxx		Add xxx to the list of "null argument" typenames that get stripped from the
#								trailing portion of template argument lists. (See also the initializations of
#								variable @nullargs). xxx must be fully qualified (including namespace).
#
#								if xxx is 'clear', the null argument list is emptied; subsequent -nullarg
#								specification appearing on the command line will work, but not the defaults
#								as per the initialization of @nullargs below (unless they're re-specified
#								using the -nullarg option later on the commmad line.)
#
#	Options supporting long typename wrapping:
#
#								(Note: If output width is set to 0, line wrapping is disabled and the
#								following options have NO EFFECT)
#
#			-break:x			Break algorithm: D[ave Abrahams] (default) or P[lain]
#								The "Dave" option wraps long complex typenames in a way that
#								makes it easier to see parameter lists at various nesting depths. 
#
#			-cbreak:x			Comma break: B = break before commas (default), A = break after
#								[applies only in -break:D mode]
#
#			-closewrap:x		Wrap before unmatched closing delimiters: Y (default) or N
#								[applies only in -break:D mode]
#
#			-meta:x				Configure for metaprogramming [Y] or vanilla wrapping [N] as follows:
#
#								-meta:y (or just -meta) forces -break:D, and forces -cbreak and
#								-closewrap options according to values specified in the $meta_y_cbreak
#								and $meta_y_closewrap variable initializations, respectively.
#
#								-meta:n forces -break:P (-cbreak and -closewrap don't apply)
#
#								In either case, if the output width hasn't yet been set to a non-zero
#								value, it is set to 80 (choosing a wrapping flavor makes no sense
#								with wrapping disabled); this may be overridden by a subsequent -width
#								option.
#
#								Note: If no -meta option is present, the default values for -break,
#								-cbreak and -closewrap are determined by the user-configurable
#								settings of $break_algorithm, $comma_wrap and $close_wrap below.
#
#								Note also that -meta is not configurable from the INI files, because
#								it is intended as a command-line "override" mechanism. The individual
#								settings of -break, -cbreak and -closewrap, however, *are* (and apply
#								only when the /meta option is not used.)
#
#################################################################################################################
# User-configurable settings (use UPPER CASE ONLY for all alphabetics here (except $newiter):
#
							####################################################################################
							# The following twelve settings may be overridden by options on
							# the command line (either explicitly, or when conveyed by the
							# Proxy C++ from settings in Proxy-gcc.INI):

							# default iterator policy (/iter:x option):
$def_iter_policy = 'L';		# 'L' (Long): NEVER remove iterator type name qualification
							# 'M' (Medium): USUALLY remove iterator type name qualification
							#               leave intact when iter type may be significant
							#				to the diagnostic
							# 'S' (Short): ALWAYS remove iterator type name qualification

$def_cand_policy = 'L';		# default candidate policy (/cand:x option):
							# 'L' (long): Retain candidate list (same as: 'Y')
							# 'M' (medium): Suppress candidates, but tell how many were suppressed
							# 'S' (short): completely ignore candidate list (same as: 'N')

$def_hdr_policy = 'LD2';	# default standard header messages policy (/hdr:x option):
							# 'L' (long): Retain all messages referring to standard header files (same as 'Y')
							# 'M' (medium): Retain only the first $headers_to_show messages in each cluster
							# 'S' (short): Discard all messages referring to standard header files (same as 'N')
							# 'LD[1]' (long, Dave Abrahams re-ordering mode 1):
							#			actual error msg moves to front of instantiation backtrace
							# 'LD2' (long, Dave Abrahams re-ordering mode 2):
							#			as above, with trigger line duplicated before the backtrace

$def_with_policy = 'S';		# default "with clause" substitution policy (/with:x option):
							# 'L' (long): Do NOT substitute in template parameters in "with" clauses
							# 'S' (short): Do substitute template parameters in "with" clauses

$def_path_policy = 'S';		# default long path policy (/path:x option):
							# 'L' (long): Retain entire pathname in all cases (same as 'Y') 
							# 'S' (short): Discard all except base name from pathnames (same as 'N')
							# [NOTE: Use L if you rely upon your IDE to locate errors in source files]

$def_hdrs_to_show = 1;		# Default number of header messages to show for 'M' header policy

$banner = 'Y';				# Show banner with Decryptor ID by default (/banner option)

$break_algorithm = 'D';		# P[lain] or D[ave Abrahams] line-breaking algorithm (/break:x option)
							# (Note: line-breaking of any kind happens only if $output_width is non-zero)

$comma_wrap = 'B';			# wrap lines B[efore] or A[fter] commas. (/cbreak:x option)
							# (Applies in Dave mode only.)

$close_wrap = 'Y';			# Force a break before close delimiters
							# whose open is not on the same line  (/closewrap:x option)
							# (Applies in Dave mode only.)

$output_width = 80;			# wrap at 80 columns by default (/width:nn option)

$show_backtraces = 'Y';		# show (Y) or suppress (N) backtrace messages (/showback:x option)


							#####################################################################################
							# The remaining settings are controlled strictly by their assigned
							# value below (no corresponding command-line options offered):

$newiter = 'iter';			# ('iter' or 'IT' or...) shorten the word "iterator" to this
							# Note: /iter:L forces $newiter to be 'iterator' (no filtering)

$tabsize = 4;				# number of chars to incrementally indent lines

$advise_re_policy_opts = 1;	# remind folks they can use /hdr:L and /cand:L to see more message details

$nix_only_once = 1;			# suppress "undefined identifiers shown only once..." messages

$reformat_linenumbers = 0;	# 1 to reformat lines numbers to LZ's preferred style (may conflict with
							# cursor-placement mechanisms in some editors/IDE's)

$smush_amps_and_stars = 0;	# 1 leaves asterisks/ampersands adjacent to preceding identifiers;
							# 0 inserts a space between

$space_after_commas = 0;	# 1 to force spaces after commas, 0 not to

$meta_y_cbreak = 'B';		# /meta:y forces this value for cbreak
$meta_y_closewrap = 'Y';	# /meta:y forces this values for closewrap

$wrap_own_msgs = 0;			# wrap STL Decryptor messages to same output width
							# as errors (1) or don't (0)

$keep_stdns = 0;			# 0 to remove "std::" and related prefixes, 1 to retain them.
							# NOTE: if set to 1, STL-related filtering will *not* work (for now). This option
							# is designed for use in conjunction with /break:D to retain maximum detail in
							# metaprogramming-style messages, rather than for use with STL library messages.

$show_internal_err = 1;		# If set to 0, suppresses delimiter mismatch errors. Please leave at 1,
							# and contact me as per the emitted instructions in the case of an
							# internal error. 

							# default list of names of trailing type names to be stripped from the end of
							# argument lists (the -nullarg:xxx command line option allows additional names to
							# be added to the list, or for the these default ones to be cleared out):

@nullargs = qw(boost::tuples::null_type mpl_::void_ mpl_::na boost::detail::variant::void\d+);
							# (Designed primarly for use with boost libraries tuple, mpl, etc...)

#@nullargs = qw();			# to totally disable the null args stripping feature by default, uncomment this
							# line and comment out the initialization of @nullargs above.


#
# END of user-configurable settings (change anything below here AT YOUR OWN RISK.)
#################################################################################################################


$| = 1;											# force output buffer flush after every line

$iter_policy = $def_iter_policy;				# default iterator policy

$dave_move = 0;									# true for /hdr:LD1
$dave_rep = 0;									# true for /hdr:LD2
$in_backtrace = 0;								# not processing a backtrace right now

if ("\u$def_hdr_policy" =~ 'LD[12]?')
{
	$dave_move = 1;
	$dave_rep = 0;
	$dave_rep = 1 if "\u$def_hdr_policy" eq 'LD2';
	$def_hdr_policy = 'L';
}

$header_policy = $def_hdr_policy;				# default std. header message policy
$with_policy = $def_with_policy;				# default "with clause" substitution policy
$headers_to_show = $def_hdrs_to_show;			# number of headers to show for 'M' header policy 
$candidate_policy = $def_cand_policy;			# default candidate policy (gcc only)
$pathname_policy = $def_path_policy;			# default pathname policy (gcc only)

$lognative = 0;									# by default, not logging native messages
$suppressed_headers = 0;						# true when we've suppressed at least one stdlib header
$suppressed_candidates = 0;						# true when we've suppressed at least one template candidate

$pdbg = 0;										# true to show print trace
$wrapdbg = 0;									# de-bug wrap loop
$movedbg = 0;									# de-bug Dave mode reordering
$delimdbg = 0;									# de-bug /wrap:D mode delimiter parsing
$optdbg = 0;									# show value of all command-line modfiable options
$choked = 0;									# haven't choked yet with an internal error


sub scanback;
sub println;
sub print2;
sub break_and_print;
sub break_and_print_plain;
sub break_and_println_plain;
sub break_and_print_fragment;
sub lognative_header;
sub showkey;


# Little hack to avoid "Exiting subroutine via next" warnings on internal error:
sub NoWarn
{
	$msg = shift (@_);
	print "$msg" if $msg !~ /Exiting subroutine via next/;
}

$SIG{__WARN__} = "NoWarn";

@save_args = @ARGV;								# lognative_header uses this

while (@ARGV)									# process command-line options
{
	if ($ARGV[0] =~ /^[\/-]iter:([SML])[A-Z]*$/i) # allow command-line iterator policy
	{											  # specification of form: /iter:x
		print "$ARGV[0] " if $optdbg;
		$iter_policy = "\u$1";
		$newiter = 'iterator' if $iter_policy eq 'L';
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]cand:([YNSML])[A-Z]*$/i) # allow candidate policy
	{												# specification of form: /cand:x
		print "$ARGV[0] " if $optdbg;
		$candidate_policy = "\u$1";
		$candidate_policy =~ tr/NY/SL/;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]with:([YNSL])[A-Z]*$/i) # allow with clause policy
	{											   # specification of form: /with:x
		print "$ARGV[0] " if $optdbg;
		$with_policy = "\u$1";
		$with_policy =~ tr/NY/LS/;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]hdr:L(D[12]?)$/i)	# detect special "Dave Abrahams" header policy
	{											# specifications of form: /hdr:LD /hdr:LD1 /hdr:LD2
		print "$ARGV[0] " if $optdbg;
		$header_policy = "L";			
		$dave_move = 1;							# moving error line to before the trace
		$dave_rep = 0;
		$dave_rep = 1 if "\u$1" eq "D2";		# replicating trigger line
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]hdr:([YNSML])[a-ce-z]*$/i)	# allow standard header policy
	{													# specification of form: /hdr:x
		print "$ARGV[0] " if $optdbg;
		$header_policy = "\u$1";
		$header_policy =~ tr/NY/SL/;
		$headers_to_show = 0 if $header_policy eq 'S';
		$dave_move = 0;
		$dave_rep = 0;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]hdr:(\d+)/i)				 # allow standard header policy
	{												 # specification of form: /hdr:nn
		print "$ARGV[0] " if $optdbg;
		$header_policy = 'M';
		$headers_to_show = $1;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]path:([SL])[A-Z]*$/i)	 # allow pathname policy
	{												 # specification of form: /path:x
		print "$ARGV[0] " if $optdbg;
		$pathname_policy = "\u$1";
		$pathname_policy =~ tr/NY/SL/;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]banner:?([YN]?)[A-Z]*$/i)	# banner:Y or N
	{												    # (just "/banner" means Y)
		print "$ARGV[0] " if $optdbg;
		$banner = "\u$1";
		$banner = 'Y' if $banner eq "";
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]showback:?([YN]?)[A-Z]*$/i)	# showback:Y or N
	{												    # (just "/showback" means Y)
		print "$ARGV[0] " if $optdbg;
		$show_backtraces = "\u$1";
		$show_backtraces = 'Y' if $show_backtraces eq "";
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]width:(\d+)/i)			# allow line output width spec
	{												# of form: /width:n
		print "$ARGV[0] " if $optdbg;
		$output_width = $1;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]break:([DP])[A-Z]*$/i)	# break: D or P
	{
		print "$ARGV[0] " if $optdbg;
		$break_algorithm = "\u$1";
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]cbreak:([AB])[A-Z]*$/i)  # comma break: B or A
	{
		print "$ARGV[0] " if $optdbg;
		$comma_wrap = "\u$1";
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]closewrap:?([YN]?)[A-Z]*$/i)  # closewrap:Y or N
	{												     # (just "/closewrap" means Y)
		print "$ARGV[0] " if $optdbg;
		$close_wrap = "\u$1";
		$close_wrap = 'Y' if $close_wrap eq "";
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]nullarg:(.*)/i)  # add to list of "null arg" identifiers
	{
		print "$ARGV[0] " if $optdbg;
		if ($1 =~ /^clear$/i)
		{
			@nullargs = ();						# "clear" means clear out null arg list
		}
		else
		{
			push @nullargs, $1;					# any other name is appened to list
		}
		shift;
		next;
	}


	if ($ARGV[0] =~ /^[\/-]meta:?([YN]?)[A-Z]*$/i)		 # meta:Y or N
	{												     # (just "/meta" means Y)
		if ("\u$1" =~ /^N/)
		{
			$break_algorithm = 'P';
		}
		else
		{
			$break_algorithm = 'D';
			$comma_wrap = $meta_y_cbreak;
			$close_wrap = $meta_y_closewrap;
		}
		$output_width = 80 if $output_width == 0;
		shift;
		next;
	}

	if ($ARGV[0] =~ /^[\/-]lognative/i)				# allow log native msgs option
	{												# of form: /lognative
		print "$ARGV[0] " if $optdbg;
		$lognative = 1;
		shift;
		next;
	}

	println "Warning: unrecognized gSTLFilt2.pl command line option: $ARGV[0]\n";
	shift;
}

# List of "standard" header files names:

@keywords = qw(algo algorithm algobase bitset cassert cctype cerrno cfloat ciso646
	climits clocale cmath complex csetjmp csignal cstdarg stdarg cstddef stddef
	cstdio stdio cstdlib stdlib cstring ctime time cwchar cwtype cxxabi deque editbuf exception exception_defines
	fstream fstream.h functional hashtable hash_map hash_set heap iomanip
	ios iosfwd iostream iostream istream iterator limits list locale
	map memory new new numeric ostream queue rope set slist
	pair sstream stack stdexcept streambuf streambuf_iterator string
	strstream strstream stream_iterator typeinfo utility valarray vector
	basic_ios basic_string boost_concept_check char_traits codecvt concept_check
	cpp_type_traits fpos functexcept generic_shadow gslice gslice_array indirect_array
	ios_base localefwd stringfwd locale_facets mask_array pthread_allocimpl slice slice_array
	type_traits valarray_array valarray_meta bastring complext fcomplex ldcomplex
	std_valarray straits strfile tempbuf alloc floatio math
	array random regex type_traits tuple unordered_map unordered_set
	cfenv cinttypes cstdbool cstdint ctgmath
 );


# Put 'em in a hash for rapid searching:

for (@keywords)
{
	$keywords{$_}++;
}


print "\n" if $optdbg;
$tab = " " x $tabsize;

print "\n\n" . ("=" x $output_width) . "\n\n" if $pdbg or $wrapdbg or $movedbg or $delimdbg;

#
# This sections builds the $t ("type") regex from the ground up. 
# After it is built, the component variables (except for $id) are not used again.
#

$sid = '\b[a-zA-Z_]\w*';						# pattern for a simple identifier or keyword
$id = "(?:$sid\:\:)*$sid";						# simple id preceded by an optional namespace qualifier
$p = '(?: ?\*)*';								# suffix for "ptr", "ptr to ptr", "ptr to ptr to ptr", ad nauseum.
$idp = "(?:$id )*$id ?$p ?";					# one or more identifiers/keywords with perhaps some *'s after

												# simple id or basic template spec
$cid = "(?:$idp(?: ?const ?\\*? ?)?|$id<$idp(?: ?const ?\\*? ?)?(?:, ?$idp(?: ?const ?\\*? ?)?)*>$p) ?";

												# a cid or template type with 1+ cid's as parameters
$t = "(?:$cid|$id<$cid(?:, ?$cid)*>$p|$id<$id<$cid>$p(?:, ?$id<$cid>$p)* ?>$p)";

println "$STLFilt_ID" if ($banner eq 'Y');

showkey $output_width if $pdbg;

lognative_header if $lognative;

$doing_candidates = 0;
$doing_stl_headers = 0;
$save_filename = '';
$this_is_gcc3 = 0;
$saw_instantiated = 0;

#
# Data structures supporting the Dave Abrahams mode line break algorithm:
#

@open_delims = ('(', '{', '<');
@close_delims= (')', '}', '>');

for (@open_delims)	# list of "open" delimiters
{
	$open_delims{$_}++;
}

for (@close_delims)	# list of "close" delimiters
{
	$close_delims{$_}++;
}

# create "opposites" table, mapping each delimiter to its complement:
for ($i = 0; $i < @open_delims; $i++)
{
	$opps{$open_delims[$i]} = $close_delims[$i];
	$opps{$close_delims[$i]} = $open_delims[$i];
}


# The following state varaibles are used for Dave-mode line re-ordering:

$pushed_back_line = "";
$displaying_error_msg = 0;				# true if displaying actual error message of backtrace in re-order mode
$last_within_context = "";				# save "within this context" lines to detect duplicates and strip them

#
# NOTE: We cannot use a main loop of the form
#
# while( <> )
#
# because of ActivePerl's way of handling input from  Win32 pipes 
# connected to STDIN. (EOF is treated like an ordinary character. 
# In particular, it doesn't get read unless FOLLOWED by a newline.
# Yeah, great, EOF followed by a newline.)
#

MAIN_LOOP:
while ( 1 )
{
  if ($pushed_back_line ne "")
  {
	  $_ = $pushed_back_line;
	  $pushed_back_line = "";
  }
  else
  {
	  # Read the first char of the next line to see if it equals EOF.
	  # If we're the ones who write the code that writes to STDIN,
	  # we can guarantee that EOF is always preceded by a newline.
	  #
	  # We must do this in a loop, because if the next line is empty,
	  # then we have not read the first char of the next line, but 
	  # the entire next line.
	  #
	  $newlines = "";

	  CHECK_FOR_EOF_LOOP: 
	  while( 1 )
	  {
		# Read one char.
		$nextchar = "";
		$numRead = read STDIN, $nextchar, 1;

		# Normally, perl will return an undefined value from read if the next
		# character was EOF. ActivePerl will simply read the EOF like any other
		# character. Since we know that one of the newlines was ours, we print one 
		# less newline than we have seen. NOTE: It is possible that we have seen no 
		# newline at all. This happens if the CL output has no newline at the end. 
		# In that case, we have appended a newline, and that's good.

		if (1 != $numRead or $nextchar eq "\032") 
		{
		  if ($newlines ne "")
		  {
			chop $newlines;
			print $newlines;
		  }
		  last MAIN_LOOP;
		}
		else    # Else, if we have read a newline, we store it for later output and continue reading.
		{
		  if ($nextchar eq "\n")
		  {
			$newlines = $newlines . "\n";
		  }

		  # Else, if we have read something that's neither a newline nor EOF, we print
		  # the accumulated newlines and proceed to read and process the next line.

		  else
		  {
			print $newlines;
			last CHECK_FOR_EOF_LOOP;
		  }
		}
	  }

	  # Read the next line, prepend the first char, which has already been read.
	  $_ = <STDIN>;

	  # If the read failed, the pipe must have broken.
	  if (!defined $_)
	  {
		print "\nSTL Decryptor: Input stream terminated abnormally (broken pipe?)\n";
		last MAIN_LOOP;
	  }

	  $_ = $nextchar . $_;
  }


#
# Done with special EOF-processing-enabled input handling.
# Now process the line (in $_):
#

  $save_line_for_dbg = $_;					# in case of a panic error

  print LOGNATIVE $_ if $lognative;			# log native message if requested

  print "DBG: Next line to be processed:\n$_\n" if $movedbg;

  s/{anonymous}/anonymous/g;				# massage anonymous namespace specs to qualify as identifiers
  s/<unnamed>/unnamed/g;

  $obj = 0;									# by default, not object file message
  $obj = 1 if /\.o(bj)?\b/i or /\.a\b/i;	# set $obj if an object or lib file

  if ($obj)
  {
	print "************** PRINT DBG 1 **************\n" if $pdbg;
	if (/(.*\):\s*)([\S].*)/)				# look for possibly-mangled name followed by additional text...
	{
		$before = $1;
		$after = "$2\n";
		break_and_print_plain "$before";	# print the part containing possible mangled-name-from-hell
		$_ = $after;						# and process the rest normally.
	}
	else
	{
		break_and_print_plain "$_";			# print entire line containing possible mangled-name-from-hell
		next;
	}
  }

  next if $show_backtraces eq 'N' and (/\binstantiated from\b/ or /^\s*from /);

# get rid of useless messages from gcc

  if ($nix_only_once)
  {
	  next if /\(Each undeclared identifier is reported only once/;
	  next if /for each function it appears in\.\)/;
  }

  $has_lineno = 0;
  $has_lineno = 1 if /^(.*\.(cpp|cxx|cc|h|hpp|tcc)\:(\d+\:) ?)/;

													# strip prefix of form: 
  if (/^(.*\.(cpp|cxx|cc|h|hpp|tcc)\:(\d+\:)? ?)/ or	#   "pathname.h:n: " or
	 (/^((?:[a-zA-Z]\:)?[^:]*\b(\w+)(\.\w+)?\:(\d+\:)? ?)/ and exists $keywords{$2}))    # std header
  {													
	$prefix = "$1";									# and restore it later
	s/^\Q$prefix//;									# remove during filtering
  }
  else
  {	
	$prefix = "";									# null prefix if none detected
  }


# test if we're looking at a message referring to a standard header
# (note that the candidate policy trumps the header policy):

  $needs_stripping = ($prefix =~ /stl_[a-zA-Z]\w*\.h:/i or
		($prefix =~ m#([/\\])include\1(?:.*\1)*(\w+)# and exists $keywords{$2}));

  $this_is_an_stl_header = ($needs_stripping and
				!$doing_candidates and ($_ !~ /candidates are\:/));
  
# strip message referring to standard headers if header_policy is S:

  if ($this_is_an_stl_header)
  {
	++$doing_stl_headers;
	next if $header_policy eq 'S';			# skip all headers if using 'S' header policy
											# skip all but first $headers_to_show if policy is 'M'
	next if $doing_stl_headers > $headers_to_show and $header_policy eq 'M';
  }

											# if 'M' or 'S' header policy, tell how many skipped:
  if (!$this_is_an_stl_header and ($doing_stl_headers > ($headers_to_show)) and $header_policy ne 'L')
  {
	println "    [STL Decryptor: Suppressed " .
					($doing_stl_headers - $headers_to_show) . 
					($header_policy eq 'M' ? " more " : " ") .
					"STL standard header message" .
					(($doing_stl_headers - $headers_to_show) != 1 ? "s" : "") . "]";
	$suppressed_headers = 1;
  }

  $doing_stl_headers = 0 if !$this_is_an_stl_header;


# gcc: strip pathname from deadly-long stdlib file paths as per $pathname_policy:

  if ($pathname_policy eq 'S' and $needs_stripping)
  {
	$preprefix = "";					# special case: preserve leading " from " phrase
	if ($prefix =~ /^(.* from )/)
	{
		$preprefix = $1;
		$prefix =~ s/^.* from //;
	}

	if ($prefix =~ /$sid\.(h|hpp|tcc|cpp|cxx|cc)\:/)	
	{												# case where there's an extension	
		$prefix =~ s/.*($sid\.(h|hpp|tcc|cpp|cxx|cc))\:/$preprefix$1:/;
	}
	else
	{										# case where there's (perhaps) no extension
		$prefix =~ s/^(?:[a-zA-Z]\:)?[^:]*\b(\w+(\.\w+)?\:(\d+\:)? ?)/$preprefix$1/;
	}
  }

# save filename, if any

  if ($prefix =~ /^($sid\.(h|hpp|tcc))\:/)
  {
	$filename = $1;					# save file basename
  }
  else
  {
	$filename = 'No filename';
  }

  if ($candidate_policy ne 'L' and /candidates are:/)
  {
	$doing_candidates++;
	$save_filename = $filename;
	$suppressed_candidates = 1;
	next;
  }

  $this_is_gcc3 = 1 if $doing_candidates and /          /;

								# skip all messages referring to "candidates" (if req'd):
  if ($candidate_policy ne 'L' and $doing_candidates and 
		($this_is_gcc3 and /          /) or (!$this_is_gcc3 and $filename eq $save_filename))
  {
	$doing_candidates++;
	next;
  }

  if (((!$this_is_gcc3 and $filename ne $save_filename) or $this_is_gcc3) and $doing_candidates)
  {
	println "    [STL Decryptor: Suppressed $doing_candidates 'candidate' line" . 
									($doing_candidates != 1 ? "s" : "") . "]"
			if $candidate_policy eq 'M';
	$doing_candidates = 0;
  }

  s/no matching function for call to/No match for/;

  ###################################################################################################
  # Do 'with' clause processing, transforming into plain-Jane type specifications:

  if ($with_policy eq 'S')
  {
	  # temporary eliminate [##] sequences:
	  @dims = ();
	  $dim_counter = 1;

	  while (/(\[(\d*)])/)
	  {
		  $old = $1;
		  $sub = $2;
		  s/\Q$old/zzz-$dim_counter-$sub-zzz/;
		  push @dims, $sub;
		  $dim_counter++;
	  }

	  while (/(.*)( \[with ([^]]*)])/)
	  {
		  $text = $1;					# the original message text with placeholder names
		  $keyclause = $2;				# the "with [...]" clause
		  $keylist = $3;				# just the list of key/value mappings

		  chop $keylist if substr($keylist, -1, 1) eq ']';

		  %map = ();					# clear the hash of key/value pairs

		  while($keylist =~ /(\w+) ?=/)
		  {
			  $key = $1;
			  $pos = $start = index($keylist, $key) + length($key) + 1;
			  if (substr($keylist, $pos, 1) eq '=')
			  { $pos++; $start++; }
			  if (substr($keylist, $pos, 1) eq ' ')
			  { $pos++; $start++; }


			  $depth = 0;				# count <'s and >'s
			  $previous = ' ';
			  while ($pos <= length($keylist))
			  {
				  $next = substr ($keylist, $pos++, 1);
				  last if $depth == 0 and ($next eq ',' or ($next eq ']' and $previous ne '[')); # ignore "[]"
				  $previous = $next;
				  $depth++ if $next =~ /[<\[\(]/;
				  $depth-- if $next =~ /[>\]\)]/;
			  }

			  $value = substr($keylist, $start, $pos - $start - 1);
			  $map{$key} = $value;

			  last if $pos > length($keylist);
			  $keylist = substr($keylist, $pos);
		  }

	  # Apply substitutions to the original text fragment:

		  $newtext = $text;
		  while(($key, $value) = each(%map))
		  {
			$newtext =~ s/\b$key\b/$value/g;
		  }

	  # Replace the original message text with the expanded version:

		  s/\Q$text/$newtext/;

	  # Delete the key/value list from the message:

		  s/\Q$keyclause//;
	  }

	  # restore [###] clauses:
	  $dim_counter = 1;
	  foreach $dim (@dims)
	  {
		  s/zzz-$dim_counter-$dim-zzz/[$dim]/g;
		  $dim_counter++;
	  }
  }

  # End 'with' clause processing
  #############################################################################################

# eliminate standard namespace qualifiers (for now, required for STL filtering):

  s/\bstd(ext)?\:\://g unless $keep_stdns;
  s/\b__gnu_cxx\:\://g unless $keep_stdns;			

  s/note: //;								# WTF? It's just noise.
  s/typename //g;							# ditto

# The following section strips out the "class" keyword when it is part
# of a type name, but not when it is part of the 'prose' of a message.
# To do this, we only strip the word "class" when it follows an
# odd-numbered single quote (1st, 3rd, 5th, 

  $out = "";								# accumulate result into $out
  $old = $_;

  while (1)
  {
	if (($pos = index ($old, "`")) == -1)	# index of next opening quote
	{										# if none, 
		$out .= $old;						# we're done
		last;
	}
	$out .= substr($old, 0, $pos + 1, "");	# splice up to & including the "'" to $out
	$pos = index($old, "'");				# index of next closing quote in $out
	$txt = substr($old, 0, $pos + 1, "");	# splice from $old into $txt
	$txt =~ s/\bclass //g if !/\btypedef\b/;	# filter out "class" from $txt
	$out .= $txt;							# concatenate result to cumulative result
  }											# loop for next fragment
  $_ = $out;								# done; update entire current line


#  s/\bclass //g if !/typedef/;			# strip "class" except in typedef errors


  s/\bstruct ([^'])/$1/g if !/typedef/;	# don't strip "struct" for *anonymous* structs or typedefs

  s/\b_STLD?\:\://g unless $keep_stdns;			# for STLPort

# simplify the ubiquitous "vanilla" string and i/ostreams (w/optional default allocator):

  s/\b(_?basic_(string|if?stream|of?stream|([io]?stringstream)))<(char|wchar_t), ?(string_)?char_traits<\4>(, ?__default_alloc_template<(true|false), ?0>)? ?>\:\:\1/$2::$2/g;
  s/\b_?basic_(string|if?stream|of?stream|([io]?stringstream))<(char|wchar_t), ?(string_)?char_traits<\3>(, ?__default_alloc_template<(true|false), ?0>)? ?>/$1/g;
  s/\b(_?basic_(string|if?stream|of?stream|([io]?stringstream)))<(char|wchar_t), ?(string_)?char_traits<\4>(, allocator<\4>)? ?>\:\:\1/$2::$2/g;
  s/\b_?basic_(string|if?stream|of?stream|([io]?stringstream))<(char|wchar_t), ?(string_)?char_traits<\3>(, allocator<\3>)? ?>/$1/g;
  s/\b([io])stream_iterator<($t), ?($t), ?char_traits<\3>, ?($t)>/$1stream_iterator<$2>/g;

  s/\b__normal_iterator<const $t, ?($t)>\:\:__normal_iterator\(/string::const_iterator(/g;
  s/\b__normal_iterator<$t, ?($t)>\:\:__normal_iterator\(/string::iterator(/g;


# The following loop repeats until no transformations occur in the last complete iteration:

  for ($pass = 1; ;$pass++)			# pass count (for de-bugging purposes only)
  {
	my $before = $_;				# save the current line; keep looping while changes happen

#
# Handle allocator clauses:
#

# delete allocators from template typenames completely:

	$has_double_gt = 0;
	$has_double_gt++ if />>/;

	s/allocator<($t)>::rebind<\1>::other::($id)/allocator<$1>::$2/g;
	s/\b,? ?allocator<$t ?>(,(0|1|true|false)) ?>/$2>/g;
	s/, ?allocator<$id<($t), ?allocator<\1> ?> ?>//g;
	s/, ?allocator<$t> ?>/>/g;


# remove allocator clauses

# gcc 4.x allocator types

	s/, ?allocator<($t) ?>\:\:rebind<\1 ?>\:\:other>/>/g;

# remove allocator clauses if the message doesn't refer to an allocator explicitly:

	unless (/' to '.*allocator</ or /allocator<$t>\:\:/)
	{
 	  s/, ?allocator<$t ?> ?//g;					# the leading comma allows the full spec.
	  s/, ?const allocator<$t ?> ?&//g;				# to appear in the error message details
 	  s/,? ?(const )?$t\:\:allocator_type ?&//g;
	}


	if (!$has_double_gt)
	{
		while (/>>/)
		{
			s/>>/> >/g;
		}
	}


# gcc deque, deque iterators:

	s/\bdeque<($t),0>/deque<$1>/g;
	s/\b_Deque_iterator<($t), ?\1 ?&, ?\1 ?[*&](, ?0)?>\:\:_Deque_iterator ?\(/deque<$1>::iterator(/g;
	s/\b_Deque_iterator<($t), ?\1 ?&, ?\1 ?[*&](, ?0)?>/deque<$1>::iterator/g;
	
	s/\b_Deque_iterator<($t), ?const \1 ?&, ?const \1 ?[*&](,0)?>\:\:_Deque_iterator ?\(/deque<$1>::const_iterator(/g;
	s/\b_Deque_iterator<($t), ?const \1 ?&, ?const \1 ?[*&](,0)?>/deque<$1>::const_iterator/g;

# gcc list iterators:

	s/\b_List_iterator<($t), ?(const )?\1 ?&, ?(const )?\1 ?\*>\:\:_List_iterator ?\(/list<$1>::iterator(/g;
	s/\b_List_iterator<($t), ?(const )?\1 ?&, ?(const )?\1 ?\*>/list<$1>::iterator/g;
	
	s/\b_List_iterator<($t)>\:\:_List_iterator\(/list<$1>::iterator(/g;
	s/\b_List_iterator<($t)>/list<$1>\:\:iterator/g;

	s/\b_List_const_iterator<($t)>\:\:_List_const_iterator ?\(/list<$1>::const_iterator(/g;
	s/\b_List_const_iterator<($t)>/list<$1>::const_iterator/g;

	s/_List_node<($t)> ?\*/list<$1>::iterator/g;
	s/_List_node_base ?\*/iterator/g;
	s/_List_const_iterator/const_iterator/g;
	s/_List_iterator/iterator/g;
	
# gcc slist iterators:
	
	s/\b_Slist_iterator<($t), ?\1 ?&, ?\1 ?\*>\:\:_Slist_iterator ?\(/slist<$1>::iterator(/g;
	s/\b_Slist_iterator<($t), ?const \1 ?&, ?const \1 ?\*>\:\:_Slist_iterator ?\(/slist<$1>::const_iterator(/g;
	s/\b_Slist_iterator<($t), ?\1 ?&, ?\1 ?\*>/slist<$1>::iterator/g;
	s/\b_Slist_iterator<($t), ?const \1 ?&, ?const \1 ?\*>/slist<$1>::const_iterator/g;
	s/\b_Slist_node<($t)> ?\*/slist<$1>::iterator/g;


# gcc vector iterator:

	s/\b__normal_iterator<($t) ?\*, vector<\1 ?> ?>\:\:__normal_iterator ?\(/vector<$1>::iterator(/g;
	s/\b__normal_iterator<($t) ?\*, vector<\1 ?> ?>/vector<$1>::iterator/g;
	s/\b__normal_iterator<($t) const ?\*, vector<\1 ?> ?>\:\:__normal_iterator ?\(/vector<$1>::const_iterator(/g;
	s/\b__normal_iterator<const ($t) ?\*, ?vector<\1 ?> ?>\:\:__normal_iterator ?\(/vector<$1>::const_iterator(/g;
	s/\b__normal_iterator<($t) const ?\*, vector<\1 ?> ?>/vector<$1>::const_iterator/g;
	s/\b__normal_iterator<const ($t) ?\*, ?vector<\1 ?> ?>/vector<$1>::const_iterator/g;


# gcc map:

    s/\b_Rb_tree<($t), ?pair<const \1, ?($t)>, ?_Select1st<pair<const \1, ?\2> ?> ?>/map<$1,$2>/g;
    s/\b_Rb_tree<($t), ?pair<const \1, ?($t)>, ?_Select1st<pair<const \1, ?\2> ?>, ($t) ?>/map<$1,$2,$3>/g;

#   s/\b_Rb_tree<($t), ?pair<const \1, ?($t)>, ?_Select1st<pair<const \1, ?\2> > ?>/map<$1,$2>/g;
    s/\b_Rb_tree<($t), ?pair<const \1, ?($t)>, ?_Select1st<pair<const \1, ?\2> ?>, ($t)::rebind<pair<const \1, ?\2> >::other>/map<$1,$2,$3>/g;


# gcc map/multimap iterators:

    s/\b_Rb_tree<($t), ?pair<const \1, ?($t)>, ?_Select1st<pair<const \1, ?\2> ?>::rebind<pair<const \1, ?\2> >::other>/multimap<$1,$2>/g;


	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t) ?> ?>\:\:_Rb_tree_iterator\(const _Rb_tree_iterator<pair<const \1 ?, ?\2 ?> ?>&\)/gen_map<$1,$2>::iterator(const gen_map<$1,$2>::iterator &)/g;	
	s/\b_Rb_tree_const_iterator<pair<const ($t) ?, ?($t) ?> ?>\:\:_Rb_tree_const_iterator\(const _Rb_tree_const_iterator<pair<const \1 ?, ?\2 ?> ?>&\)/gen_map<$1,$2>::const_iterator(const gen_map<$1,$2>::const_iterator &)/g;	
	s/\b_Rb_tree_const_iterator<pair<const ($t) ?, ?($t) ?> ?>\:\:_Rb_tree_const_iterator\(const _Rb_tree_iterator<pair<const \1 ?, ?\2 ?> ?>&\)/gen_map<$1,$2>::const_iterator(const gen_map<$1,$2>::iterator &)/g;	

	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t) ?> ?>\:\:_Rb_tree_iterator\(_Rb_tree_node<pair<const \1, ?\2 ?> ?>\*\)/gen_map<$1,$2>::iterator/g;	
	s/\b_Rb_tree_const_iterator<pair<const ($t) ?, ?($t) ?> ?>\:\:_Rb_tree_const_iterator\(const _Rb_tree_node<pair<const \1, ?\2 ?> ?>\*\)/gen_map<$1,$2>::iterator/g;	

	s/\b_Rb_tree_iterator<pair<($t) ?const, ?($t) ?> ?>/gen_map<$1,$2>::iterator/g;
	s/\b_Rb_tree_const_iterator<pair<($t) ?const, ?($t) ?> ?>/gen_map<$1,$2>::const_iterator/g;

	s/\b_Rb_tree_node<pair<const ($t), ?($t) ?> ?> ?\*/gen_map<$1,$2>::iterator/g;

	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t)>, ?pair<const \1, ?\2> ?&, ?pair<const \1, ?\2> ?\*>\:\:_Rb_tree_iterator ?\(/gen_map<$1,$2>::iterator(/g;
	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t)>, ?pair<const \1, ?\2> ?&, ?pair<const \1, ?\2> ?\*>/gen_map<$1,$2>::iterator/g;

	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t)>, ?const pair<const \1, ?\2> ?&,const pair<const \1, ?\2> ?\*>\:\:_Rb_tree_iterator ?\(/gen_map<$1,$2>::const_iterator(/g;
	s/\b_Rb_tree_iterator<pair<($t) ?const, ?($t)>, ?pair<\1 ?const, ?\2> ?&, ?pair<\1 ?const, ?\2> ?\*>\:\:_Rb_tree_iterator ?\(/gen_map<$1,$2>::const_iterator(/g;

	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t)>, ?const pair<const \1, ?\2> ?&,const pair<const \1, ?\2> ?\*>/gen_map<$1,$2>::const_iterator/g;
	s/\b_Rb_tree_iterator<pair<($t) ?const, ?($t)>, ?pair<\1 ?const, ?\2> ?&, ?pair<\1 ?const, ?\2> ?\*>/gen_map<$1,$2>::const_iterator/g;

	s/\b_Rb_tree_iterator<pair<const ($t) ?, ?($t)> ?>/gen_map<$1,$2>::iterator/g;
	s/\b_Rb_tree_const_iterator<pair<const ($t) ?, ?($t)> ?>/gen_map<$1,$2>::const_iterator/g;
	

# gcc set/multiset/map/multimap iterator (who knows, lol?)
	
	s/\b_Rb_tree_iterator\(($t)\)/gen_set_or_map::iterator($1)/g;
	s/\b_Rb_tree_const_iterator<($t)>\:\:_Rb_tree_const_iterator\(/gen_set_or_map<$1>::const_iterator(/g;
	s/\b_Rb_tree_const_iterator<($t)>/gen_set_or_map<$1>::const_iterator/g;
	s/\b_Rb_tree_const_iterator\(($t)\)/gen_set_or_map::const_iterator($1)/g;


# Since the same iterator type is used for both set and multiset, we just
# say "gen_set<T>::iterator" to mean the "GENERIC" set/multiset iterator type:

# gcc set/multiset:

	s/\b_Rb_tree<($t), ?\1, ?_Identity<$t>, ?($t)>/gen_set<$1,$2>/g;
	s/\b_Rb_tree<($t), ?\1, ?_Identity<$t>>/gen_set<$1>/g;


# gcc set/multiset iterator:

	s/\b_Rb_tree_iterator<($t), ?const \1 ?&, ?const \1 ?\*>::_Rb_tree_iterator ?\(/gen_set<$1>::iterator(/g;
	
	s/\b_Rb_tree_iterator<($t), ?const \1 ?&, ?const \1 ?\*>/gen_set<$1>::iterator/g;

	s/\b_Rb_tree_iterator<($t) \*, ?\1 ?\*const &, ?\1 ?\*const \*>::_Rb_tree_iterator ?\(/gen_set<$1>::iterator(/g;
	s/\b_Rb_tree_iterator<($t) \*, ?\1 ?\*const &, ?\1 ?\*const \*>/gen_set<$1>::iterator/g;

	s/[`']\b_Rb_tree_const_iterator ?<($t)>\:\:_Rb_tree_const_iterator\(const _Rb_tree_node<($t)> ?\*\)'/gen_set<$1>::const_iterator/g;

	s/[`']\b_Rb_tree_iterator ?\(($t)\)'/gen_set<$1>::iterator/g;
	s/[`']\b_Rb_tree_const_iterator ?\(($t)\)'/gen_set<$1>::const_iterator/g;
	s/\b_Rb_tree_iterator ?\(($t)\)/gen_set<$1>::iterator/g;
	s/\b_Rb_tree_const_iterator ?\((const $t)\)/gen_set<$1>::const_iterator/g;
	s/\b_Rb_tree_iterator ?<($t)>/gen_set<$1>::iterator/g;
	s/\b_Rb_tree_const_iterator ?<($t)>/gen_set<$1>::const_iterator/g;
	
	s/\bconst _Rb_tree_node<($t) ?> ?\*/gen_set<$1>::const_iterator/g;
	s/\b_Rb_tree_node<($t)> ?\*/gen_set<$1>::iterator/g;

	
# STLPort /gcc hash_set/_multiset:

	s/\b(hash_(?:multi)?)set<($t)(, ?hash<\2 ?>)? ?(, ?equal_to<\2 ?>)? ?>/$1set<$2>/g;
	s/\bhashtable<($t), ?\1, ?hash<\1 ?>, ?_Identity<\1 ?>, ?$t>/gen_hash_set<$1>/g;


# gcc hash_map/multimap:

	s/\b(hash_(?:multi)?)map<($t), ?($t), ?hash<\2 ?>, ?($t)>/$1map<$2,$3>/g;


# gcc hash_set/hash_multiset iterator:

	s/\b_Hashtable_const_iterator<($t), ?\1, ?hash<\1 ?>, ?_Identity<\1 ?>, ?$t>\:\:_Hashtable_const_iterator ?\(/gen_hash_set($1):const_iterator(/g;
	s/\b_Hashtable_const_iterator<($t), ?\1, ?hash<\1 ?>, ?_Identity<\1 ?>, ?$t>/gen_hash_set($1):const_iterator/g;
	s/\b_Hashtable_iterator<($t), ?\1, ?hash<\1 ?>, ?_Identity<\1 ?>, ?$t>\:\:_Hashtable_iterator ?\(/gen_hash_set($1):iterator(/g;
	s/\b_Hashtable_iterator<($t), ?\1, ?hash<\1>, ?_Identity<\1>, ?$t>/gen_hash_set($1):iterator/g;

# gcc hash_map/hash_multimap:
	
	s/\bhashtable<(pair<const ($t), ?($t)>), ?\2, ?hash<\2 ?>, ?_Select1st<\1 ?>, ?$t ?>/gen_hash_map<$2,$3>/g;

# gcc hash_set/hash_multiset value_type:

	s/\bhashtable<($t), ?\1, ?hash<\1>, ?_Identity<\1>, ?$t>::value_type/$1/g;

# STLPort/gcc hash_map/_multimap iterators under VC7 (same "GENERIC" iterator approach as above):

	s/\b_Hashtable_iterator<pair<const ($t), ?($t)>, ?\1, ?hash<\1 ?>, ?_Select1st<pair<const \1, ?\2> ?>, ?equal_to<\1 ?> ?>\:\:_Hashtable_iterator\(/gen_hash_map<$1,$2>::iterator(/g;
	s/\b_Hashtable_const_iterator<pair<const ($t), ?($t)>, ?\1, ?hash<\1 ?>, ?_Select1st<pair<const \1, ?\2> ?>, ?equal_to<\1 ?> ?>\:\:_Hashtable_const_iterator\(/gen_hash_map<$1,$2>::const_iterator(/g;
	s/\b_Hashtable_iterator<pair<const ($t), ?($t)>, ?\1, ?hash<\1 ?>, ?_Select1st<pair<const \1, ?\2> ?>, ?equal_to<\1 ?> ?>/gen_hash_map<$1,$2>::iterator/g;
	s/\b_Hashtable_const_iterator<pair<const ($t), ?($t)>, ?\1, ?hash<\1 ?>, ?_Select1st<pair<const \1, ?\2> ?>, ?equal_to<\1 ?> ?>/gen_hash_map<$1,$2>::const_iterator/g;

# simplify default comparison function objects, leave others intact:

	s/, ?_?less<$t ?>//g;
	s/, ?Comp<$t ?>//g;				# STLPort's default comparison function

	last if $before eq $_;			# keep looping if substitutions were actually made
  }

# reverse iterators:

  s/\breverse_iterator<($t)::iterator ?>/$1::reverse_iterator/g;
  s/\breverse_iterator<($t)::const_iterator ?>/$1::const_reverse_iterator/g;
  s/\bconst_reverse_iterator\:\:reverse_iterator/const_reverase_iterator/g;

# reduce iterators according to $iter_policy:

  $olditer = '(reverse_)?(bidirectional_)?((back_)?insert_)?iterator';
  if ($iter_policy eq 'M')					# policy 'M': USUALLY remove:
  {
	unless (/( of type|' to '|from ')$t\:\:(const_)?$olditer/  # Shorten to $newiter and
			or /iterator' does/)							   # *remove* the base type completely...
	{														   # as long as the error message doesn't	
		s/$t\:\:((const_)?$olditer)\b/$1/g;					   # mention iterators!				
	}			
  }
  elsif ($iter_policy eq 'S')								# policy 'S': ALWAYS remove:
  {
	s/$t\:\:((const_)?$olditer)\b/$1/g;						#	remove the base type completely
  }
															# All policies (including 'L'):
  s/\biterator\b/$newiter/g;
  s/\b(const_|reverse_|const_reverse_)iterator\b/$1$newiter/g; 

# remove trailing "null args" from template parameter lists:

  foreach $name (@nullargs)
  {
	  s/(, ?$name ?)* ?>/>/g;
  }

# reduce "double" constructor names 'T::T' to just 'T':

  s/[`']string\:\:string([\('])/'string$1/g;
  s/[`'](.*)\:\:\1([\('])/'$1$2/g;

# deal with some other non-critical (and often not even very aesthetic) spaces (or lack thereof):

  s/>>>>>([\(:',*&])/> > > > >$1/g;		# put spaces between the close brackets
  s/>>>>([\(:',*&])/> > > >$1/g;
  s/>>>([\(:',*&])/> > >$1/g;

  if (/(.)>>(.)/)						# careful, ">>" could be operator...
  {
	  $before = $1;
	  $after = $2;
	  s/(.)>>([ \(:',*&])/$1> >$2/g unless (/operator ?>>/ or ($before eq ' ' and $after eq ' '));
  }


  s/([^> ]) >([^>=])/$1>$2/g;							# remove space before '>' (unless between another '>' or '>=')

  s/([\w>])([&*])/$1 $2/g if !$smush_amps_and_stars;	# conditionally force space between identifier and '*' or '&'

  s/,([^ ])/, $1/g if $space_after_commas;				# add space *after* a comma, however, if desired.


  s/   initializing argument / init. arg /;

# and FINALLY, print out the result of all transformations, preceded by saved prefix:

  $_ = $prefix . $_;
  s/\:                /:/ if $break_algorithm eq 'D';	# lose "candidate" alignment in "Dave" wrap mode

  # reformat line number indicator
  s/\:(\d+)/($1)/g if $reformat_linenumbers;

  if ($dave_move)
  {
    print "DBG: A\n" if $movedbg;
	$has_from = /^\s*from /;
	if (!$in_backtrace)
	{
		print "DBG: B\n" if $movedbg;
		if (/:\s+In\b/i or /^\s*In file included from/)
		{
			$in_backtrace = 1;					# template instantiation backtrace?
			print "DBG: C (in_backtrace set to 1)\n" if $movedbg;
			@backtrace = ();
		}
	}
	elsif (/instantiated\s+from\s+here/)
	{
		print "DBG: D\n" if $movedbg;
		$saved_trigger = $_;
	}
	elsif (/instantiated\s+from\s+[`']/)		#` Fix syntax hilighting for emacs
	{
		$saw_instantiated = 1;
		print "DBG: E (saw_instantiated set to 1)\n" if $movedbg;
	}
	elsif (/^\s*from .*[,:]$/)
	{
		$in_backtrace = 2;						# header backtrace
		print "DBG: F (in_backtrace set to 2)\n" if $movedbg;
	}
	else										# we've come to the end.
	{
		print "DBG: G\n" if $movedbg;

		# if we've already begun displaying the final error message:
		
		if ($displaying_error_msg)
		{
			if (substr($_, 0, 1) =~ /\s/ or /within this context/)		# if we see a line beginning with whitespace, it is a continuation of the actual error message:
			{
				if (/within this context/)
				{
					next if $last_within_context eq $_;			# strip consecutive identical "within this context" lines (gcc bug?)
					$last_within_context = $_;
				}
				break_and_print $_;
				next;
			}
			else			# if we see a line not starting with whitespace, finalize the re-order mode processing:
			{
				$last_within_context = "";				# reset "within this context" duplicate detection mechanism
				break_and_print (shift @backtrace);				# print 1st line of backtrace

														# if replicating trigger, do it:
				break_and_print $saved_trigger if $dave_rep and $saw_instantiated;

				$backtrace[$#backtrace] =~ s/:$//		# if header file backtrace, strip trailing
					if $in_backtrace == 2 and @backtrace;				# colon from last line

				foreach $line (@backtrace)				# now emit the accumulated backtrace
				{
					break_and_print $line;
				}
				print "\n";
				$in_backtrace = 0;
				print "DBG: I\n" if $movedbg;

				$displaying_error_msg = 0;
				$last_within_context = "";
				$pushed_back_line = $_;
				next;
			}
		}
		
		if ($saw_instantiated or ($in_backtrace == 2 and $has_lineno and !$has_from))
		{
			print "DBG: H\n" if $movedbg;
			print "************** PRINT DBG 2 **************\n" if $pdbg;
			print "\n";								# let's set the "deep" error sequence apart
			break_and_print $_;						# print the first line of the actual error first
			$displaying_error_msg = 1;				# and go into displaying error message mode until we see a line beginning with non-ws
			next;
		}
		elsif ($in_backtrace == 1)					# template backtrace "false alarm"?
		{
			print "DBG: J\n" if $movedbg;
			break_and_print (shift @backtrace);		# yes.
			$in_backtrace = 0;						# reset for another round
		}
	}

	print "DBG: K\n" if $movedbg;
	if ($in_backtrace)
	{
		print "DBG: L\n" if $movedbg;
		push @backtrace, $_;
		next;
	}
  }

  print "************** PRINT DBG 3 **************\n" if $pdbg;
  break_and_print "$_";
}


if ($displaying_error_msg)
{
	break_and_print (shift @backtrace);		# print 1st line of backtrace

											# if replicating trigger, do it:
	break_and_print $saved_trigger if $dave_rep and $saw_instantiated;

	$backtrace[$#backtrace] =~ s/:$//		# if header file backtrace, strip trailing
		if $in_backtrace == 2 and @backtrace;				# colon from last line

	foreach $line (@backtrace)				# now emit the accumulated backtrace
	{
		break_and_print $line;
	}
	print "\n";

	print "DBG: I2\n" if $movedbg;
}

close LOGNATIVE if $lognative;					# close native messages logfile if active

if ($doing_candidates and $candidate_policy eq 'M')
{
	println "    [STL Decryptor: Suppressed $doing_candidates 'candidate' line" . 
									($doing_candidates != 1 ? "s" : "") . "]";
}
else
{
								# if 'M' or 'S' header policy, tell how many skipped:
	if (($doing_stl_headers > $headers_to_show) and $header_policy ne 'L')
	{
		println "    [STL Decryptor: Suppressed " . 
					($doing_stl_headers - $headers_to_show) .
					($header_policy eq 'M' ? " more " : " ") .
					"STL standard header message" . 
					(($doing_stl_headers - $headers_to_show) != 1 ? "s" : "") . "]";
		$suppressed_headers = 1;
	}
}

if ($advise_re_policy_opts and ($suppressed_headers or $suppressed_candidates))
{
	print "\nSTL Decryptor reminder";
	print "s" if $suppressed_headers and $suppressed_candidates;
	println ":";
	println "    Use the /hdr:L option to see all suppressed standard lib headers" if $suppressed_headers;
	println "    Use the /cand:L option to see all suppressed template candidates" if $suppressed_candidates;
}

if ($choked)
{
	print "\n***************************************************************************\n";
	print "A non-fatal, internal STL Decryptor error has occurred.\n";
	if  ($show_internal_err)
	{
		print "It should have said as much somewhere above, and then emitted the \n";
		print "partially-filtered line.\n";
	}
	print "Please look for a file just created named NativeLog.txt,\n";
	print "and email this file to leor\@bdsoft.com. This will greatly assist me\n";
	print "to understand and try to correct the problem. Thank you!\n";
	print "***************************************************************************\n";
}

exit 0;


sub break_and_print {
	my $line = shift(@_);

	if ($output_width == 0 or ($break_algorithm eq 'P' and length($line) < $output_width))
	{
		$line =~ s/\s+\n/\n/g;	# delete trailing space on a line
		print "************** PRINT DBG 4 **************\n" if $pdbg;
		print "$line";
		return;
	}

	if ($break_algorithm eq 'P')
	{
		print "************** PRINT DBG 5 **************\n" if $pdbg;
		break_and_print_plain "$line";	
		return;
	}

	$nesting_level = 0;			# track combined nesting level for () [] <> {}
	$in_quotes = 0;				# not in quotes

WRAPLOOP:
	for ($frag_count = 0; ;$frag_count++)
	{
		print "\nDBG: Top of WRAPLOOP, line to process is: '$line'\n" if $wrapdbg;
		print "DBG: top of WRAPLOOP a: frag_count = $frag_count, nesting_level = $nesting_level\n" if $wrapdbg;
		print "DBG: tabsize = $tabsize, in_quotes = $in_quotes\n" if $wrapdbg;

		$indentation = $nesting_level;		# save indentation at start of every line
		$width = $output_width - ($nesting_level * $tabsize);

		print "DBG: top of WRAPLOOP: indentation = $indentation\n" if $wrapdbg;
		print "DBG: width now $width \n" if $wrapdbg;
		
		$line =~ s/^\s*//;				# delete leading spaces
		$line =~ s/,([^ \t])/, $1/g;	# make sure commas are followed by a  space for gcc2


		if ($frag_count > 0)			# make sure only 1st line of message hugs left margin
		{
			$indentation++;
			$width -= $tabsize;
		}

		$at_left = 0;					# recognize when there's no nested parens
		$at_left = 1 if ($close_wrap eq 'N' and
						($frag_count == 0 or ($frag_count > 0 and $nesting_level == 1)));

		# Preprocess line, creating table mapping close- to open-parens:

		print "\n\nDBG: line to process (width = $width, nesting_level = $nesting_level, in_quotes = $in_quotes):\n$line" if $pdbg or $delimdbg;
		showkey $width if $pdbg or $delimdbg;


        $first_unmatched_close = $width;				# position of first unmatched close paren
        $unmatched_close_nesting = $nesting_level;      # the nesting level below which we'll consider a close paren to be unmatched
        $initial_close = 0;						        # assume first character is not a close paren
        
		@delims = ();		# list of unmatched open delims
		@delim_index = ();	# for each open, record its position
		@nesting_key = ();	# record nesting level at each char position
		@quoting_key = ();	# record whether in quotations at each char position

		# for each closer, we'll record the position of the corresponding opener in @delim_opener

		# begin by resetting each position:
		for ($i = 0; $i < length($line); $i++)
		{
			$delim_opener[$i] = -1;
		}

		for ($pos = 0; $pos < $width and $pos < length($line); $pos++)
		{
			$c = substr($line, $pos, 1);

			print "DBG: delimiter de-bugging, column pos = $pos (char there = '$c')\n" if $delimdbg;

			if (exists $open_delims{$c})
			{
				$before = ' ';
				$before = substr($line, $pos-1, 1) if $pos > 0;

				$beforetext = "";
				$beforetext = substr($line, $pos - 8, 8) if ($pos >= 8);	# looking for "operator"

				$after = ' ';
				if ($pos < (length($line) - 1))
				{
					$after = substr($line, $pos+1, 1);
					$aftertext = substr($line, $pos+1);
				}
																				# Exclude some special cases:
				if (!($before eq '`' and $after eq '\'') and					# `paren'
					!($before eq ' ' and $after eq ' ' and $c eq '<') and		# <space> < <space> (relop)
					!( ($before eq $c or $after eq $c) and ($c eq '<')) and		# two <'s in a row
					!($c eq '<' and $after eq '=') and							# <=
					!($c eq '<' and ($after eq '"' or $before eq '"')) and		# "< or <"
					!($c eq '(' and $after eq ')') and							# ()
					!(($c eq '<' or $c eq '(') and $beforetext =~ /\boperator ?<?$/) and		# operator<, operator<<, operator()
					!($aftertext =~ /^first use/)
				   )
				{
					print "DBG: matched opening delim '$c', aftertext = '$aftertext'\n" if $delimdbg;
					$nesting_level++;
					push @delims, $c;
					push @delim_index, $pos;
				}
			}
			elsif (exists $close_delims{$c})
			{
				$before = ' ';
				$before = substr($line, $pos-1, 1) if $pos > 0;

				$beforetext = "";
				$beforetext = substr($line, $pos - 10, 10) if ($pos >= 10);	# looking for "operator"

				$after = ' ';
				if ($pos < (length($line) - 1))
				{
					$after = substr($line, $pos+1, 1);
					$aftertext = substr($line, $pos+1);
				}
																				# Exclude some special cases:
				if (!($before eq '`' and $after eq '\'') and					# `paren'
					!($beforetext =~ /\s$/ and $c eq '>' and $after eq '>') and		# whitespace >>
					!($beforetext =~ /\s>$/ and $c eq '>' and $aftertext =~ /^\s/) and	# whitespace >> whitespace
					!($c eq '>' and $after eq '=') and							# >=
					!($c eq '>' and ($after eq '"' or $before eq '"')) and		# "> or >"
					!($c eq '>' and $beforetext =~ /\boperator ?>?$/) and		# operator> or operator>>
					!($c eq ')' and $before eq '(') and							# ()
					!($aftertext =~ /^first use/) and
					!($before eq '-' and $c eq '>'))							# special case: -> operator (!!)
				{
					print "DBG: matched closing delim '$c', aftertext = '$aftertext'\n" if $delimdbg;
					$nesting_level--;
					# If the nesting_level ever falls below its level at
					# the beginning of the line, we have an unmatched
					# close paren and we must force a break there.
					if ($pos == 0)
					{
						 # initial close delimiters don't count, we'll break after the nesting falls again
						$initial_close = 1;
						$unmatched_close_nesting--;
					}
					elsif ($close_wrap eq 'Y' and $in_quotes 
						   and $first_unmatched_close > $pos
						   and $nesting_level < $unmatched_close_nesting)
					{
						$first_unmatched_close = $pos;
					}

					if (@delims > 0)
					{
						if ($c ne $opps{$delims[$#delims]})
						{
							if ($show_internal_err)
							{
								if (!$lognative)
								{
									lognative_header;
									print LOGNATIVE "Raw unprocessed input line:\n$save_line_for_dbg\n\n";		# write out the unprocessed offending line							}
									$lognative = 1;
								}

								print LOGNATIVE "\nThe line at the point of the error was:\n$line\n";
								print LOGNATIVE " " x $pos . "^\n";
								print LOGNATIVE "\nNesting key: " . "@nesting_key\n";

								print LOGNATIVE "\nSTL Decryptor ERROR: the char '$c' (position $pos) DOESN'T MATCH DELIMITER '$delims[$#delims]'!\n";

								$choked = 1;
								print "\n";
								print "       [An Internal STL Decryptor error has occurred while processing\n";
								print "        the line that follows. Note that the line has not been successfully\n";
								print "        wrapped, but substitutions should still be intact:]\n\n";
							}
							print "$line\n";		# write out the unwrapped line
							next MAIN_LOOP;
						}
						else
						{
							pop @delims;
							$delim_opener[$pos] = pop @delim_index;	# map close index to open index
						}
					}
				}
			}
			elsif ($c =~ /[`']/)
			{
				$in_quotes = !$in_quotes;
			}
            $nesting_key[$pos] = $nesting_level;	# track nesting level at each column
            $quoting_key[$pos] = $in_quotes;		# track nesting level at each column
		}

		# STEP 0: If there's an unmatched back-quote before the end of the line,
		# and we're not in col. 1, and it isn't just `id', wrap before the back-quote:

		if (length($line) > $width and substr($line, 0, $width-1) =~ /(`$id[^'`]*$)/)  #` Fix syntax highlighting for emacs
		{
			$name_pos = $-[0];

			if ($name_pos != 0)
			{
				print "*********** PRINT DBG 6 width=$width indent=$indentation *********\n" if $pdbg;
				print2 ($indentation, (substr($line, 0, $name_pos) . "\n"));
					$line = substr($line, $name_pos);
				$nesting_level = $nesting_key[$name_pos - 1];
				$in_quotes = $quoting_key[$name_pos - 1];
				next WRAPLOOP;
			}
		}

		# STEP 1: If there's an incomplete paren pair on the current line:

		if (@delims)
		{
			#
			# STEP 1A: Find the first open paren on the line whose matching close paren doesn't fit:
			#
			$open_pos = $delim_index[0];

			while (1)
			{
				$beforeit = substr($line, 0, $open_pos);

				# STEP 1Ai: If it's immediately preceded by >::ident[::ident...]
				#                                        or foo.template bar
				#										 or foo.bar(...).template baz

				if (($beforeit =~ /[>)](\s*(::|\.|->)?\s*$id)+$/) and $-[0] != 0)  
				{
					# STEP 1A1a: If the opening angle bracket matching the 1st char of the r.e. above is on
					#			 this line, move back to that position and go to 1Ai
					$open_pos = $-[0];
					if ($delim_opener[$open_pos] != -1)
					{
						$open_pos = $delim_opener[$open_pos];
						next;
					}
					# STEP 1A1b: Else move to the first char of the r.e. above and go on to 1B (this is
					#			 already where $open_pos indicates)
				}
				last;
			}

            if ($first_unmatched_close < $open_pos)
            {
				print "******** PRINT DBG 6A width=$width indent=$indentation ******\n" if $pdbg;
                print2  ($indentation, (substr($line, 0, $first_unmatched_close) . "\n"));
                $line = substr($line, $first_unmatched_close);
                $nesting_level = $nesting_key[$first_unmatched_close - 1];
				$in_quotes = $quoting_key[$first_unmatched_close - 1];
                next WRAPLOOP;
            }

			#
			# STEP 1B: If we're on an open paren, and there's a comma earlier on the line at the same level,
			#		   and the last such comma is not the first non-ws char on the line, wrap just before it
			#		   and don't indent:
			#
			$c = substr($line, $open_pos, 1);
			if (exists $open_delims{$c})	# if it's an open paren...
			{
				if ($in_quotes and scanback "$line")
				{
					# We've found the comma
					if ($comma_wrap eq 'B' and $nesting_level > 0)
					{
						# Wrap just before $comma_pos:
						print "******** PRINT DBG 7 width=$width indent=$indentation ******\n" if $pdbg;
						print2 ($indentation, (substr($line, 0, $comma_pos) . "\n"));
						$line = substr($line, $comma_pos);
						$nesting_level = $nesting_key[$comma_pos - 1];
						$in_quotes = $quoting_key[$comma_pos - 1];
					}
					else
					{
						# Wrap just after $comma_pos:
						print "******** PRINT DBG 8 width=$width indent=$indentation ******\n" if $pdbg;
						print2  ($indentation, (substr($line, 0, $comma_pos +1) . "\n"));
						while (substr($line, $comma_pos + 1, 1) eq ' ')
						{
							$comma_pos++;
						}
						$line = substr($line, $comma_pos + 1);
						$nesting_level = $nesting_key[$comma_pos];
						$in_quotes = $quoting_key[$comma_pos];
					}
					next WRAPLOOP;
				}
			}

			# STEP 1C: Else if it's an open paren:
			#			if $nesting_level is 0, wrap before the pattern
			#			if line begins at col. 1, wrap just after the open and indent the next line

			$c = substr($line, $open_pos, 1);
			if (exists $open_delims{$c})	# if it's an open paren...
			{
				print "*********** PRINT DBG 9 width=$width indent=$indentation *********\n" if $pdbg;
				print2 ($indentation, (substr($line, 0, $open_pos + 1) . "\n"));
				$width -= $tabsize;
				$line = "  " . substr($line, $open_pos + 1);	# special case xtra leading indent
				$nesting_level = $nesting_key[$open_pos];
				$in_quotes = $quoting_key[$open_pos];
				next WRAPLOOP;
			}
			else		# It must be a close paren. Wrap just before it and unindent the next line:
			{
				print "*********** PRINT DBG 10 width=$width indent=$indentation *********\n" if $pdbg;
				print2 ($indentation, (substr($line, 0, $open_pos) . "\n"));
				$width += $tabsize;
				$line = substr($line, $open_pos);
				$nesting_level = $nesting_key[$open_pos - 1];
				$in_quotes = $quoting_key[$open_pos - 1];
				next WRAPLOOP;
			}
		}
        elsif ($first_unmatched_close < $pos)
        {
			print "*********** PRINT DBG 10A width=$width indent=$indentation *********\n" if $pdbg;
            print2  ($indentation, (substr($line, 0, $first_unmatched_close) . "\n"));
            $line = substr($line, $first_unmatched_close);
            $nesting_level = $nesting_key[$first_unmatched_close - 1];
			$in_quotes = $quoting_key[$first_unmatched_close - 1];
            next WRAPLOOP;
        }
		else
		{
			# STEP 2: If there is a comma at the current level of paren nesting AND
			#		  we're not at a nesting level of 0:
			$open_pos = $pos;

			if (!$at_left and $in_quotes and scanback "$line")
			{
				if ($comma_wrap eq 'B' and $nesting_level > 0)
				{
					print "*********** PRINT DBG 11: width=$width indent=$indentation *********\n" if $pdbg;
					print2 ($indentation, (substr($line, 0, $comma_pos) . "\n"));
					$line = substr($line, $comma_pos);
					$nesting_level = $nesting_key[$comma_pos - 1];
					$in_quotes = $quoting_key[$comma_pos - 1];
				}
				else
				{
					print "*********** PRINT DBG 12: width=$width indent=$indentation *********\n" if $pdbg;
					print2 ($indentation, (substr($line, 0, $comma_pos + 1) . "\n"));
					while ($comma_pos < ($width - 1) and  substr($line, $comma_pos + 1, 1) eq ' ')
					{
						$comma_pos++;
					}
					$line = substr($line, $comma_pos + 1);
					$nesting_level = $nesting_key[$comma_pos];
					$in_quotes = $quoting_key[$comma_pos];
				}
			}
			else
			{
				# extra by LZ: if last part of line is "`identifier" we have an unbalanced
				# nesting level, then break before the "`"

				if (length($line) > $width )
				{
					$beforeit = substr($line, 0, $open_pos);
					if ($line =~ /`$id/ or $line =~/`[+-=!*<>%^&|\/~]+/)
					{
						$name_pos = $-[0];
						if ($name_pos != 0 and $name_pos < $width and 
								!(($beforeit =~ /`$id'/ or $beforeit =~ /`[+-=!*<>%^&|\/~]+'/)
									and $+[0] < $width)) #'))) fix syntax highlighting for emacs
						{
							if (exists $close_delims{substr($line, 0, 1)})	# special case for leading close
							{
								print "*********** PRINT DBG 13: width=$width indent=$indentation *********\n" if $pdbg;
								break_and_print_plain2 (($tab x ($indentation - 1)) . 
														substr($line, 0, $name_pos) . "\n");
							}
							else
							{
								print "*********** PRINT DBG 14: width=$width indent=$indentation *********\n" if $pdbg;
								break_and_print_plain2 (($tab x $indentation) . 
														substr($line, 0, $name_pos) . "\n");
							}
							$line = substr($line, $name_pos);
							$nesting_level = $nesting_key[$name_pos - 1];
							$in_quotes = $quoting_key[$name_pos - 1];
							next WRAPLOOP;
						}
					}
				}

				# STEP 3: Just break according to standard alrogithm
				print "*********** PRINT DBG 15: width=$width indent=$indentation *********\n" if $pdbg;
				$line = break_and_print_fragment ( $indentation, $width, $line);

				return if $line eq "";

				$nesting_level = $nesting_key[$broke_at];
				$in_quotes = $quoting_key[$broke_at];
			}
		}
	}	# WRAPLOOP
}


#
# break entire line using "plain" rules:
# usage: break_and_print_plain line
#

sub break_and_print_plain {
	my $line = shift(@_);
	if ($output_width != 0)
	{
		return if ($line = break_and_print_fragment (0, $output_width, "$line")) eq "";
		while (($line = break_and_print_fragment (1, $output_width - $tabsize, "$line")) ne "")
		{}
	}
	else
	{
		$line =~ s/\s+\n/\n/g;	# delete trailing space on a line
		print "$line";
	}
}


#
# Process line using "Plain" break algorithm up to first line break,
#	 return remainder of line for subsequent processing:
# usage: break_and_print_fragment indent line
#
# No matter what, don't break in the middle of a pathname...so as not
# to mess up tools that locate errors in files. If the line length has
# to exceed the /width setting, so be it in that case...
#

sub break_and_print_fragment {
	my $indent = shift(@_);
	my $width = shift(@_) - 1;
	my $line = shift(@_);

	$nl_pos = index($line, "\n");
	if ($nl_pos == -1 or $nl_pos > $width)
	{
		if ($obj)
		{
			substr($line, $width - 1, 0) = "\n" if (length($line) > $width);
		}
		else
		{
			if (length($line) > $width)
			{
				if ($prefix ne "" and  $line =~ /:\d+:\s/g and pos($line) > $width)			# never break within pathname
				{
					substr($line, pos($line), 0) = "\n";
				}
				else
				{
					$pos = $width;
					$theChar = substr($line, $pos, 1);
					$theCharBefore = ($pos > 0) ? substr($line, $pos - 1, 1) : ' ';
					$theCharAfter = substr($line, $pos + 1, 1);

					while ( ($theChar !~ /[\n '`,:]/ and $pos > 0 and !($theChar =~ /\w/ and $theCharBefore !~ /\w/))
						 or ($theChar !~ /\s/ and $theCharBefore =~ /[:\/\\.]/)
						 or ($theChar eq ':' and $theCharAfter =~ /[\/\\.]/)
						 or ($theChar eq "'" and $theCharBefore !~ /\s/)
						 or ($theChar =~ /[A-Za-z]/ and $theCharBefore =~ /['"`]/)
						  )
					{
						$pos--;
						$theCharAfter = $theChar;
						$theChar = substr($line, $pos, 1);
						$theCharBefore = ($pos > 0) ? substr($line, $pos - 1, 1) : ' ';
					}

					$pos-- if $pos > 0 and $theChar eq ':' and substr($line, $pos-1, 1) eq ':';

					if ($pos == 0)
					{
						$pos += $width;
						substr($line, $pos, 0) = "\n";
					}
					else
					{
						substr($line, $pos, $theChar eq ' ' ? 1 : 0)  = "\n";
						$pos += ($theChar eq ' ' ? 1 : 2);
					}
				}
			}
		}
	}

	$line =~ s/ +\n/\n/g;	# delete trailing space on a line

	$nl_pos = index($line, "\n");
	if ($nl_pos == -1)
	{
		print2 ($indent, $line);
		return "";
	}

	print2 ($indent, substr($line, 0, $nl_pos + 1));
	$whats_left = substr($line, $nl_pos + 1);
	$broke_at = $nl_pos - 1;
	return "$whats_left";
}


sub println
{
	if ($wrap_own_msgs)
	{
		break_and_println_plain shift(@_);
	}
	else
	{
		print shift(@_) . "\n";
	}
}


#
# call break_and_print_plain/fragment, adjust leading comma
#

sub break_and_print_plain2 {
	my $line = shift(@_);
	$line =~ s/^(\s*)  ,/$1,/;					# Adjust leading comma
	break_and_print_plain "$line";
}



#
# break_and_println_plain: Break with Plain rules, add a newline:
#

sub break_and_println_plain {
	break_and_print_plain shift(@_);
	print "\n";
}


#
# print2: prints a line
#   If using the 'Dave" break algorithm:
#	Rule 1: Omits two spaces of indentation if there's a leading comma
#   Rule 2: Reduces indentation by one tab if there's a leading close "paren"
#			 (unless already at far left)
#
# usage: print2 (indentation,line)
#

sub print2
{
	my $indent = shift(@_);
	my $line = shift(@_);
	my $prefix = "";

	if ($break_algorithm eq 'D')
	{
		if ($indent > 0)					# special case for leading close - unindent just this line
		{
			$prefix = "$tab" x 
				((exists $close_delims{substr($line, 0, 1)} and $indent > 1) ? ($indent - 1) : $indent);
		}

		if (substr($line, 0, 1) eq "," and length($line) > 4)	# if leading comma, omit two spaces
		{														# of indentation
			substr($prefix, length($prefix) - 4, 4) = "  ";
		}

		# back up one space for open quotes to make alignment pretty in /break:D mode
		if (substr($line, 0, 1) eq "`" and length($prefix) > 0)	# ?? (was '`')
		{
			substr($prefix, 0, 1) = "";
		}
	}        
	else
	{
		$prefix = "$tab" x $indent;
	}

	print "$prefix" . $line;
}


#
# Scan backwards from position $open_pos for a comma at the same paren nesting level that is
# not the first non-whitespace on the line.
# return true if found, with $comma_pos indicating the position of the detected comma.
#

sub scanback
{
	my $line = shift(@_);
	my $c = substr($line, $open_pos, 1);

	$nest_level = 0;
	for ($comma_pos = $open_pos - 1; $comma_pos > 0; $comma_pos--)	# scan back for comma at same level
	{
		$c = substr($line, $comma_pos, 1);
		if (exists $open_delims{$c})
		{								# bail if we come to an open before (working
			last if $nest_level == 0;	# to the left) its matching close
			$nest_level--;
			next;
		}
		elsif (exists $close_delims{$c})
		{
			$nest_level++;
			next;
		}
		elsif ($c eq ',' and $nest_level == 0)		# comma at same level?
		{
			for ($pos = $comma_pos - 1; $pos > 0; $pos--)	# scan to start of line
			{
				if (substr($line, $pos, 1) !~ /\s/)			# comma preceded by non-space?
				{
					return 1;								# yes, so it is a valid comma
				}
			}

			# It IS the first non-ws on the line.
			last;
		}
	}

	return 0;
}


sub lognative_header
{
	open(LOGNATIVE, ">NativeLog.txt") or
		die "STL Decryptor: Can't create NativeLog.txt. Bailing out.";

	print LOGNATIVE "---------------------------------------------------------------------\n";
	print LOGNATIVE "$STLFilt_ID\n";
	print LOGNATIVE "---------------------------------------------------------------------\n";

	print LOGNATIVE "Command line: '@save_args'\n";

	print LOGNATIVE "banner = $banner\n";
	print LOGNATIVE "break_algorithm = $break_algorithm\n";
	print LOGNATIVE "comma_wrap = $comma_wrap\n";
	print LOGNATIVE "close_wrap = $close_wrap\n";
	print LOGNATIVE "output_width = $output_width\n";
	print LOGNATIVE "tabsize = $tabsize\n";
	print LOGNATIVE "advise_re_policy_opts = $advise_re_policy_opts\n";
	print LOGNATIVE "reformat_linenumbers = $reformat_linenumbers\n";
	print LOGNATIVE "wrap_own_msgs = $wrap_own_msgs\n";
	print LOGNATIVE "header_policy = $header_policy\n";
	print LOGNATIVE "with_policy = $with_policy\n";
	print LOGNATIVE "headers_to_show = $headers_to_show\n";
	print LOGNATIVE "candidate_policy = $candidate_policy\n";
	print LOGNATIVE "pathname_policy = $pathname_policy\n";
	print LOGNATIVE "show_backtraces = $show_backtraces\n";
	print LOGNATIVE "---------------------------------------------------------------------\n";
	print LOGNATIVE "Native input follows:\n";
	print LOGNATIVE "---------------------------------------------------------------------\n\n";
}



sub showkey
{
	my $width = shift (@_);

	print  ((" " x ($width - 1)) . "v\n");
	print  "          1         2         3         4         5         6         7         8         9\n";
	print  "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345\n";
}
