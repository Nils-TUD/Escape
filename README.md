Escape
======

Escape is a UNIX-like microkernel operating system on which I'm working since
october 2008. It's implemented in C, C++ and a bit assembler. I'm trying to
write all code myself to maximize the learning effect and the fun, but
there are some cases where it just gets too time consuming to do that. Thus,
I'm using the bootloader GRUB, libgcc, libsupc++ and x86emu.  
Escape runs on x86, x86_64, ECO32 and MMIX.
[ECO32](http://homepages.thm.de/~hg53/eco32/) is a 32-bit big-endian RISC
architecture, created by Hellwig Geisse at the University of Applied Sciences
in Gießen for research and teaching purposes.
MMIX is a 64-bit big-endian RISC architecture, developed by Donald Knuth as
the successor of MIX, which is the abstract machine that is used in the famous
bookseries The Art of Computer Programming. More precisely, Escape runs only
on [GIMMIX](http://homepages.thm.de/~hg53/gimmix/), a simulator for MMIX
developed by myself for my master thesis (the differences are minimal, but
currently required for Escape).  

If you just want to take a quick look, you'll find
[screenshots](https://github.com/Nils-TUD/Escape/wiki/Screenshots) and
ready-to-use [images](https://github.com/Nils-TUD/Escape/wiki/Images) in
the wiki.


General structure
-----------------

Escape consists of a kernel, drivers, libraries and user-applications. The
kernel is responsible for processes, threads, memory-management, a virtual
file system (VFS) and some other things. Drivers run in user-space and work
via message-passing. They do not necessarily access the hardware and should
therefore more seen as an instance that provides a service.  
The VFS is one of the central points of Escape. Besides the opportunity to
create files and folders in it, it is used to provide information about the
state of the system for the user-space (memory-usage, CPU, running
processes, ...). Most important, the VFS is used to communicate with devices,
which are registered and handled by drivers. That means, each device gets
a node in the VFS over which it can be accessed. So, for example an
`open("/dev/zero",...)` would open the device "zero". Afterwards one can use
the received file-descriptor to communicate with it.  
Basically, the communication works over messages. Escape provides the
system-calls `send` and `receive` for that purpose. Additionally, Escape defines
standard-messages for `open`, `read`, `write` and `close`, which devices
may support. For example, when using the `read` system-call with a
file-descriptor for a device, Escape will send the read-standard-message to
the corresponding device and handle the communication for the user-program.
As soon as the answer is available, the result is passed back to the
user-program that called `read`.  
Because of this mechanism the user-space doesn't have to distinguish between
virtual files, "real" files and devices. So, for example the tool cat can
simply do an `open` and use `read` to read from that file and `write` to
write to stdout.  
The same mechanism is used for filesystems, which can be mounted somewhere
in the VFS. If a file is opened which belongs to one of these mounted
filesystems, the kernel will handle the communication with the driver that
has been registers as being responsible for this filesystem. This does not
only serve for getting filesystems out of the kernel, but does also provide
the possibility to use the filesystem interface whenever it fits. For example,
Escape has a ftpfs that allows you to access a FTP server by mounting it
somewhere and a tarfs to access and/or change a tar file.  

Features
--------

Escape has currently the following features:

* **Kernel**
    * Memory-management supporting copy-on-write, text-sharing,
      shared-libraries, shared-memory, mapping of physical mem (e.g. VESA-mem),
      demand-loading and swapping
    * Process- and thread-management, scheduler, ELF-loader, groups, signals,
      semaphores (are also used to pass interrupts to drivers) and
      timer-management (for sleeping and preemption)
    * SMP-support, CPU-detection and CPU-speed-calculation, FPU-support
    * Virtual file system for driver-access, accessing the filesystems,
      creating virtual files and directories and providing information about
      the system (memory-usage, cpu, running processes, ...)
    * Debugging-console
* **libc**  
  Except of some IMO not very important things like locale, math and so on,
  most of a C standard library exists by now.
* **libcpp**  
  The C++ standard library isn't that complete as the libc. But the most basic
  things like string, iostreams, vector, list, tuple, map, algorithm and a few
  other things are available (some of them are a bit simpler than the standard
  specifies it). That means, one can develop applications with the library when
  one doesn't need the more exotic stuff ;)
* **libesc**  
  Many abstractions specificly for Escape. Most important, it contains classes
  to communicate with certain types of devices like the window managers, NICs,
  TCP/IP stacks and so on. Additionally, the library provides classes to
  implement devices.
* **libgui**  
  The GUI-library isn't finished yet, but does already provide things like
  basic controls, layout-manager, image-support (BMP and PNG), event-system
  (keypresses, ...) and so on. It is written in C++, which was one of the main
  reason to provide C++-support in Escape.
* **libz**  
  Contains classes for deflate/inflate, gzip and CRC32.
* **drivers**  
  Escape has some architecture-specific drivers and some common drivers that
  are architecture- independent. Currently, there are:
    * common
        * ext2: a nearly complete ext2-rev0 fs
        * ftpfs: allows to access an FTP like an fs
        * http: presents a HTTP accessible resource as a file
        * iso9660: a iso9660 fs (cd fs).
        * lo: the loopback driver
        * pipe: realizes pipes
        * ramdisk: a fs backend that sits in RAM
        * tarfs: allows to access a tar like an fs
        * tcpip: a TCP/IP stack (v4 only atm)
        * uimng: multiplexes input- and output-devices between its clients.
          A client gets an UI which supplies him with keyboard- and mouse-
          events and provides him a screen. Both text- and graphical interfaces
          are supported. uimng has keyboard-shortcuts for creating UIs and
          switching between them.
        * vterm: A text-interface client of uimng that provides a layer of
          abstraction for user-processes that are textbased.
        * winmng: A GUI client of uimng that manages all existing windows. Gets
          keyboard- and mouse-events from uimng and forwards it to the
          corresponding application.
        * random/null/zero: Like /dev/{random,null,zero} in e.g. Linux
    * x86
        * ata: Reading and writing from/to ATA/ATAPI-devices. It supports
          PIO-mode and DMA, LBA28 and LBA48.
        * e1000: network driver for Intel e1000 cards.
        * ps2: a PS/2 driver with keyboard and mouse support. Gets notified
          about interrupts, converts scancodes to keycodes and broadcasts
          keyboard/mouse events to all clients.
        * ne2k: network driver for NE2000 compatible cards
        * pci: Collects the available PCI-devices and gives others access to
          them. This is e.g. used by ata to find the IDE-controller.
        * speaker: Produces beeps with the PC-speaker with a specified
          frequency and duration.
        * video: An output-device for uimng that supports VGA and VESA. It
          uses the BIOS via x86emu (to support x86 and x86_64) to get and set
          video modes. Clients establish a shared-memory for the screen and
          talk via messages with the driver to copy parts of it to the actual
          framebuffer.
    * ECO32/MMIX  
      Both architectures have the same devices atm (the only difference is that
      ECO32-devices are accessed with 32-bit words and MMIX-devices with 64-bit
      words).
        * disk: the disk-driver and therefore the equivalent to ata on x86.
        * keyb: the keyboard-driver
        * video: the VGA-driver
* **user**  
  There are quite a few user-programs so that you can actually do something with
  Escape ;) The most important ones are:
    * initloader: Is loaded as boot-module and is used as the first process.
      Loads the other boot-modules (disk-driver, pci (on x86 only) and fs
      because we need them to load other drivers and user-apps from the disk)
      and mounts the root filesystem. After that it replaces itself with init.
    * init: Is responsible for loading the drivers. Which drivers should be
      loaded is determined by /etc/drivers.
    * dynlink: the dynamic linker of Escape that is started by the kernel, if
      the requested application is dynamically linked. It receives the fd for
      the executable, loads all dependencies, does the relocation and
      initialization and finally jumps to the application.
    * (g)login: Accepts username+password, sets uid, gid and further group-ids
      using /etc/users and /etc/groups, creates stdin, stdout and stderr, sets
      the env-vars CWD and USER and exchanges itself with the shell. There is
      one for the text interface and one for the GUI.
    * shell: The shell implements a small shell-scripting-language-interpreter
      with pipes, io-redirection, path-expansion, background-jobs, arithmetic,
      loops, if-statements, functions, variables and arrays. The interpreter
      is realized with flex and bison.
    * cat, chmod, chown, cp, cut, date, dd, dump, find, grep, groups, gunzip,
      gzip, head, kill, less, ln, ls, lscpu, lspci, mkdir, mke2fs, more, mount,
      mv, ping, ps, pstree, readelf, rm, rmdir, size, sleep, sort, stat, sync,
      tail, tar, time, top, touch, umount, users, wc: The well-known UNIX-tools.
      They don't do exactly the same and are, of course, much simpler, but in
      principle they are intended for the same things.
    * ts: analogously to ps, ts lists all threads
    * power: for reboot / shutdown using init.
    * keymap: A tool to change the current keymap (us and german is available)
    * vtctrl: A tool for changing the size of the vterm, i.e. the video-mode.
    * lsacpi: list ACPI tables
    * net-link: a tool for managing the network links (similar to ifconfig)
    * net-arp: a tool to manage the ARP tables
    * net-routes: a tool to manage the routing tables
    * dhcp: use DHCP to configure a network link
    * imginfo: show information about an image
    * desktop, fileman, gcalc, gsettings, guishell: some GUI programs
    * ...


Getting started
---------------

1. At first you need to build the cross-compiler for the desired
   architecture:  
   `$ cd cross`  
   `$ ./build.sh (i586|x86_64|eco32|mmix)`  
   This will download gcc and binutils, build the cross-compiler and put it
   into /opt/escape-cross-$arch.
2. Now you can build escape:  
   `$ cd ../source`  
   `$ export ESC_TARGET=$arch` # choose $arch, if necessary (default: i586)  
   `$ ./b`
3. To start it, you have to choose a boot-script at boot/`$ESC_TARGET`/.
   E.g.:  
   `$ ./b run qemu-cd`
4. For further information, take a look at `./b -h`

