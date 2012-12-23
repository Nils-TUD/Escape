#!/usr/bin/env python

import os
import argparse
import subprocess
import sys

DEFAULT_PARTS_START = 128

# stores a new disk to <image> with the partitions <parts>. each item in <parts> is a list of
# the filesystem, the size in MB and the directory from which to copy the content into the fs
def create_disk(image, parts, flat, nogrub):
	if len(parts) == 0:
		exit("Please provide at least one partition")
	if len(parts) > 4:
		exit("Sorry, the maximum number of partitions is currently 4")
	if flat and len(parts) != 1:
		exit("If using flat mode you have to specify exactly 1 partition")

	# default offset when using partitions
	offset = 2048 if not flat else DEFAULT_PARTS_START

	# determine size of disk
	totalmb = 0
	for p in args.part:
		totalmb += int(p[1])
	hdcyl = totalmb
	totalsecs = offset + hdheads * hdtracksecs * hdcyl
	totalbytes = totalsecs * secsize

	# create image
	subprocess.call(["dd", "if=/dev/zero" , "of=" + str(image), "bs=512", "count=" + str(totalsecs)])

	lodev = create_loop(image)
	tmpfile = subprocess.check_output("mktemp").rstrip()
	if not flat:
		# build command file for fdisk
		with open(tmpfile, "w") as f:
			i = 1
			for p in parts:
				# n = new partition, p = primary, partition number, default offset
				f.write('n\np\n' + str(i) + '\n\n')
				# the last partition gets the remaining sectors
				if i == len(parts):
					f.write('\n')
				# all others get all sectors up to the following partition
				else:
					f.write(str(block_offset(parts, offset, i) * 2 - 1) + '\n')
				i += 1
			# a 1 = make partition 1 bootable, w = write partitions to disk
			f.write('a\n1\nw\n')

		# create partitions with fdisk
		with open(tmpfile, "r") as fin:
			p = subprocess.Popen(
				["sudo", "fdisk", "-u", "-C", str(hdcyl), "-S", str(hdheads), lodev], stdin=fin
			)
			p.wait()
	
	# create filesystems
	i = 0
	for p in parts:
		off = block_offset(parts, offset, i)
		blocks = mb_to_blocks(int(p[1]))
		print "Creating ", p[0], " filesystem in partition ", i, " (@ ", off, ",", blocks, " blocks)"
		create_fs(image, off, p[0], blocks)
		i += 1

	# copy content to partitions
	i = 0
	for p in parts:
		if p[2] != "-" and p[0] != 'nofs':
			copy_files(image, block_offset(parts, offset, i), p[2])
		i += 1
	
	if not nogrub:
		# add grub
		with open(tmpfile, "w") as f:
			f.write("device (hd0) " + str(image) + "\nroot (hd0,0)\nsetup (hd0)\nquit\n")
		with open(tmpfile, "r") as fin:
			p = subprocess.Popen(["grub", "--no-floppy", "--batch"], stdin=fin)
			p.wait()

	# remove temp file
	subprocess.call(["rm", "-Rf", tmpfile])
	free_loop(lodev)

# mounts the partition in <image> @ <offset> to <dest>
def mount_disk(image, offset, dest):
	lodev = subprocess.check_output(["sudo", "losetup" , "-f"]).rstrip()
	return subprocess.call([
		"sudo", "mount", "-oloop=" + lodev + ",offset=" + str(offset * 1024), image, dest
	])

# unmounts <dest>
def umount_disk(dest):
	i = 0
	while i < 10 and subprocess.call(["sudo", "umount", "-d", dest]) != 0:
		i += 1

# determines the number of blocks for <mb> MB
def mb_to_blocks(mb):
	return (mb * hdheads * hdtracksecs) / 2

# determines the block offset for partition <no> in <parts>
def block_offset(parts, secoffset, no):
	i = 0
	off = secoffset / 2
	for p in parts:
		if i == no:
			return off
		off += mb_to_blocks(int(p[1]))
		i += 1

# creates a free loop device for <image>, starting at <offset>
def create_loop(image, offset = 0):
	lodev = subprocess.check_output(["sudo", "losetup" , "-f"]).rstrip()
	subprocess.call(["sudo", "losetup", "-o", str(offset), lodev, image])
	return lodev

# frees loop device <lodev>
def free_loop(lodev):
	# sometimes the resource is still busy, so try it a few times
	i = 0
	while i < 10 and subprocess.call(["sudo", "losetup", "-d", lodev]) != 0:
		i += 1

# creates filesystem <fs> for the partition @ <offset> in <image> with <blocks> blocks
def create_fs(image, offset, fs, blocks):
	lodev = create_loop(image, offset * 1024)
	if fs == 'ext2r0':
		subprocess.call(["sudo", "mke2fs", "-r0", "-Onone", "-b1024", lodev, str(blocks)])
	elif fs == 'ext2' or fs == 'ext3' or fs == 'ext4':
		subprocess.call(["sudo", "mkfs." + fs, "-b", "1024", lodev, str(blocks)])
	elif fs == 'fat32':
		subprocess.call(["sudo", "mkfs.vfat", lodev, str(blocks)])
	elif fs == 'ntfs':
		subprocess.call(["sudo", "mkfs.ntfs", "-s", "1024", lodev, str(blocks)])
	elif fs != 'nofs':
		print "Unsupported filesystem"
	free_loop(lodev)

# copies the directory <directory> into the filesystem of partition @ <offset> in <image>, 
def copy_files(image, offset, directory):
	tmpdir = subprocess.check_output(["mktemp", "-d"]).rstrip()
	mount_disk(image, offset, tmpdir)
	for node in os.listdir(directory):
		subprocess.call(["sudo", "cp", "--preserve=all", "-R", directory + '/' + node, tmpdir])
	umount_disk(tmpdir)
	subprocess.call(["rm", "-Rf", tmpdir])

# runs fdisk for <image>
def run_fdisk(image):
	lodev = create_loop(image)
	hdcyl = os.path.getsize(image) / (1024 * 1024)
	subprocess.call(["sudo", "fdisk", "-u", "-C", str(hdcyl), "-S", str(hdheads), lodev])
	free_loop(lodev)

# runs parted for <image>
def run_parted(image):
	lodev = create_loop(image)
	subprocess.call(["sudo", "parted", lodev, "print"])
	free_loop(lodev)

# run fsck for the partition with offset <offset> in <image>, assuming fs <fstype>
def run_fsck(image, fstype, offset):
	lodev = create_loop(image, offset * 1024)
	subprocess.call(["sudo", "fsck", "-t", fstype, lodev])
	free_loop(lodev)

def get_part_offset(image, part):
	if part == 0:
		return DEFAULT_PARTS_START / 2
	lodev = create_loop(image)
	offset = int(subprocess.check_output(
		"sudo fdisk -l " + lodev + " | grep '^" + lodev + "p" + str(part)
			+ "' | sed -e 's/\*/ /' | xargs | cut -d ' ' -f 2",
		shell=True
	).rstrip()) / 2
	free_loop(lodev)
	return offset

# subcommand functions
def create(args):
	create_disk(args.disk, args.part, args.flat, args.nogrub)
def fdisk(args):
	run_fdisk(args.disk)
def parted(args):
	run_parted(args.disk)
def fsck(args):
	offset = get_part_offset(args.disk, args.part)
	print "Running fsck for partition ", args.part, " (@", offset, ") of ", args.disk, " with fs ", args.fs
	run_fsck(args.disk, args.fs, offset)
def mount(args):
	offset = get_part_offset(args.disk, args.part)
	print "Mounting partition ", args.part, " (@", offset, ") of ", args.disk, " at ", args.dest
	mount_disk(args.disk, offset, args.dest)
def umount(args):
	umount_disk(args.dir)

# disk geometry (512 * 31 * 63 ~= 1 mb)
secsize = 512
hdheads = 31
hdtracksecs = 63

# argument handling
parser = argparse.ArgumentParser(description='This is a tool for creating disk images with'
	+ ' specified partitions. Additionally, you can mount partitions and analyze the disk'
	+ ' with fdisk and parted.')
subparsers = parser.add_subparsers(
	title='subcommands',description='valid subcommands',help='additional help'
)

parser_create = subparsers.add_parser('create', description='Writes a new disk image to <diskimage>'
	+ ' with the partitions specified with --part (at least one, at most four).')
parser_create.add_argument('disk', metavar='<diskimage>')
parser_create.add_argument('--part', nargs=3, metavar=('<fs>', '<mb>', '<dir>'), action='append',
	help='<fs> must be one of ext2r0, ext2, ext3, ext4, ntfs, fat32, nofs.'
		+ ' <mb> is the partition size in megabytes.'
		+ ' <dir> is the directory which content should be copied to the partition. if <dir> is "-"'
		+ ' nothing will be copied')
parser_create.add_argument('--flat', action='store_true',
	help='Do not create partitions. Use the disk for one fs @ offset ' + str(DEFAULT_PARTS_START))
parser_create.add_argument('--nogrub', action='store_true', help='Don\'t put grub on the disk')
parser_create.set_defaults(func=create)

parser_fdisk = subparsers.add_parser('fdisk', description='Runs fdisk for <diskimage>.')
parser_fdisk.add_argument('disk', metavar='<diskimage>')
parser_fdisk.set_defaults(func=fdisk)

parser_parted = subparsers.add_parser('parted', description='Runs parted for <diskimage>.')
parser_parted.add_argument('disk', metavar='<diskimage>')
parser_parted.set_defaults(func=parted)

parser_fsck = subparsers.add_parser('fsck', description='Runs fsck for <part> of <diskimage>, assuming <fs>.')
parser_fsck.add_argument('disk', metavar='<diskimage>')
parser_fsck.add_argument('fs', metavar='<fs>')
parser_fsck.add_argument('part', metavar='<number>', type=int)
parser_fsck.set_defaults(func=fsck)

parser_mount = subparsers.add_parser('mount', description='Mounts <part> of <diskimage> in <dir>.')
parser_mount.add_argument('disk', metavar='<diskimage>')
parser_mount.add_argument('dest', metavar='<dir>')
parser_mount.add_argument('part', metavar='<number>', type=int)
parser_mount.set_defaults(func=mount)

parser_umount = subparsers.add_parser('umount', description='Unmounts <dir>.')
parser_umount.add_argument('dir', metavar='<dir>')
parser_umount.set_defaults(func=umount)

args = parser.parse_args()
args.func(args)

