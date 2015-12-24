#!/usr/bin/env python2

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
	if binary[0:10] == '/sys/boot/':
		binary = '/sbin/' + binary[10:]
	res = subprocess.check_output(
		crossdir + "/bin/" + cross + "-nm -Cl " + builddir + "/dist/" + binary + " | grep -i ' \\(t\\|w\\) '",
		shell=True
	)
	for l in res.split("\n"):
		match = re.match('([0-9a-f]+) (?:T|t|W|w) ([^\t]+)(?:\t(.*))?$', l)
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
if sys.stdin.isatty():
	enable_echo(sys.stdin.fileno(), False)
signal.signal(signal.SIGINT, sigint_handler)

# wait for start of backtrace
while True:
	line = sys.stdin.readline()
	if not line:
		break
	if line == '============= snip =============\r\n':
		print line[:-1]
		break

# read the regions and build a code-region- and symbol-list
coderegs = []
syms = get_symbols('/boot/escape')
if target == 'x86_64':
	coderegs.append(('/boot/escape', 0xffffffff80000000, 0xffffffffffffffff, syms))
else:
	coderegs.append(('/boot/escape', 0xc0100000, 0xffffffff, syms))
while True:
	line = sys.stdin.readline()
	if not line:
		break
	if line[0:9] == 'Pagefault' or line == 'User-Stacktrace:\r\n' or line == 'Kernel-Stacktrace:\r\n':
		break

	print line[:-1]
	match = re.match('\\s*(\\S+)\\s+([0-9a-f:]+)\.\.([0-9a-f:]+) \\(\s*\\d+K\\) .*?X.*?S', line)
	if match:
		syms = get_symbols(match.group(1))
		coderegs.append((match.group(1), str_to_addr(match.group(2)), str_to_addr(match.group(3)), syms))

# decode pagefault address
match = re.match('Pagefault for address ([0-9a-f:]+) @ ([0-9a-f:]+)', line)
if match:
	pfaddr = str_to_addr(match.group(1))
	addr = str_to_addr(match.group(2))
	(lib, sym) = find_symbol(coderegs, addr)
	liboff = addr - libaddr(lib)
	funcoff = liboff - sym[0]
	sys.stdout.write("Pagefault for %#010x\n" % (pfaddr));
	sys.stdout.write("          at  %#010x (%s+%#x): %s+%#x (%s)\n"
		% (addr, lib[0], liboff, sym[1], funcoff, sym[2]));
else:
	print line[:-1]

# decode backtrace
while True:
	line = sys.stdin.readline()
	if not line:
		break
	match = re.match('\\s*([0-9a-f:]+) -> ([0-9a-f:]+)', line)
	if match:
		addr = str_to_addr(match.group(2))
		(lib, sym) = find_symbol(coderegs, addr)
		liboff = addr - lib[1]
		funcoff = liboff - sym[0]
		sys.stdout.write("\t=>: %#010x (%s+%#x)\n" % (addr, lib[0], liboff))
		sys.stdout.write("\t    %s+%#x (%s)\n" % (sym[1], funcoff, sym[2]))
	elif line == '============= snip =============\r\n':
		print line[:-1]
		break
	else:
		print line[:-1]

if sys.stdin.isatty():
	enable_echo(sys.stdin.fileno(), True)
