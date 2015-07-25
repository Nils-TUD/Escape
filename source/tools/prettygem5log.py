#!/usr/bin/env python2

import os, sys, subprocess, re

if len(sys.argv) < 4:
	print >>sys.stderr, "Usage: ", sys.argv[0], "(all|sum) <log> <binary>..."
	sys.exit(1)

def compare_funcs(a, b):
	# hack: sometimes it's represented as an int and sometimes as a long. so, just assume that ints
	# are always smaller than longs (the function needs to return an int)
	if type(a[0]) != type(b[0]):
		return int(-1) if type(a[0]) == type(0) else 1
	return int(a[0] - b[0])

def funcname(funcs, addr):
	for f in funcs:
		if addr >= f[0] and addr < f[0] + f[1]:
			return (f[2], "\033[1m%-10s\033[0m @ %-30s" % (f[3], "%s+%d" % (f[2].split('(')[0], addr - f[0])))
	return ("<Unknown>", "\033[1m%-10s\033[0m @ %-30s" % ("<Unknown>", "%x" % (addr)))

funcs = []

regex = re.compile('([a-f0-9]+) ([a-f0-9]+) (t|T) (.+)')
for arg in sys.argv[3:]:
	name = os.path.basename(arg)
	proc = subprocess.Popen(["nm", "-SC", arg], stdout=subprocess.PIPE)
	for line in iter(proc.stdout.readline,''):
		m = re.match(regex, line.rstrip())
		if m != None:
			funcs.append([int(m.group(1), 16), int(m.group(2), 16), m.group(4), name])

funcs = sorted(funcs, cmp=compare_funcs)

count = 0
laststart = 0
lasttime = 0
lastfunc = (0, '')
regex = re.compile('\s*(\d+): (\S+) (\S+) : 0x([a-f0-9]+)\.?\s*\d*\s*:\s*(.*)$')
with open(sys.argv[2], "r") as f:
	for line in f:
		m = re.match(regex, line.rstrip())
		if m != None:
			(addr, func) = funcname(funcs, int(m.group(4), 16))
			if sys.argv[1] == 'sum':
				if lastfunc[1] != '' and addr != lastfunc[0]:
					print "@%s (%8d): %s %s : %s" % (laststart, count, m.group(2), m.group(3), lastfunc[1])
					laststart = int(m.group(1))
					count = 0
				lastfunc = (addr, func)
				count += int(m.group(1)) - lasttime
				lasttime = int(m.group(1))
			else:
				print "%s: %s %s : %s %s" % (m.group(1), m.group(2), m.group(3), func, m.group(5))
		else:
			print line.rstrip()
