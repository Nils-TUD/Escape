#!/usr/bin/env python

import sys, os, subprocess, re, termios, signal

target = os.environ.get('ESC_TARGET', 'i586')
build = os.environ.get('ESC_BUILD', 'release')
cross = target + '-elf-escape'
crossdir = '/opt/escape-cross-' + target
builddir = 'build/' + target + '-' + build

# enables/disables echo for the terminal
def enable_echo(fd, enabled):
	(iflag, oflag, cflag, lflag, ispeed, ospeed, cc) = termios.tcgetattr(fd)
	if enabled:
		lflag |= termios.ECHO
	else:
		lflag &= ~termios.ECHO
	new_attr = [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]
	termios.tcsetattr(fd, termios.TCSANOW, new_attr)

# determines the dynamic address of the given library (or 0 for programs)
def libaddr(lib):
	return lib[1] if "/lib/" in lib[0] else 0

# searches for <addr> in <coderegs> and returns (<lib>, <sym>)
def find_symbol(coderegs, addr):
	for r in coderegs:
		if addr >= r[1] and addr < r[2]:
			off = libaddr(r)
			for s in r[3]:
				if addr - off >= s[0]:
					return (r, s)
	return (('Unknown', 0, 0), (0, 'Unknown', None))

# returns an array of (<addr>, <symbol>, <lineinfo>) for all symbols in the given binary
def get_symbols(binary):
	symbols = []
	res = subprocess.check_output(
		crossdir + "/bin/" + cross + "-nm -Cl " + builddir + "/dist/" + binary + " | grep -i ' t '",
		shell=True
	)
	for l in res.split("\n"):
		match = re.match('([0-9a-f]+) (?:T|t) ([^\t]+)(?:\t(.*))?$', l)
		if match:
			symbols.append((int(match.group(1), 16), match.group(2), match.group(3)))
	symbols.sort(key=lambda tup: tup[0], reverse=True)
	return symbols

# converts '1234:5678' to an int
def str_to_addr(string):
	string = string.replace(':', '')
	return int(string, 16)

# signal handler to ensure that we reenable echo in the terminal
def sigint_handler(signum, frame):
	enable_echo(sys.stdin.fileno(), True)
	sys.exit(1)

# don't echo the stuff the user pasted
enable_echo(sys.stdin.fileno(), False)
signal.signal(signal.SIGINT, sigint_handler)

# read the regions and build a code-region- and symbol-list
coderegs = []
while True:
	line = sys.stdin.readline()
	print line[:-1]
	match = re.match('\\s*(\\S+)\\s+([0-9a-f:]+) - ([0-9a-f:]+) \\(\s*\\d+K\\):.*?Ex.*', line)
	if match:
		syms = get_symbols(match.group(1))
		coderegs.append((match.group(1), str_to_addr(match.group(2)), str_to_addr(match.group(3)), syms))
	elif line == 'User-Stacktrace:\n':
		break

# decode backtrace
while True:
	line = sys.stdin.readline()
	match = re.match('\\s*([0-9a-f:]+) -> ([0-9a-f:]+) \\(.*?\\)', line)
	if match:
		addr = str_to_addr(match.group(2))
		(lib, sym) = find_symbol(coderegs, addr)
		liboff = addr - libaddr(lib)
		funcoff = liboff - sym[0]
		sys.stdout.write("\t=>: %#010x (%s+%#x)\n" % (addr, lib[0], liboff))
		sys.stdout.write("\t    %s+%#x (%s)\n" % (sym[1], funcoff, sym[2]))
	elif line == '============= snip =============\n':
		print line[:-1]
		break
	else:
		print line[:-1]

enable_echo(sys.stdin.fileno(), True)

