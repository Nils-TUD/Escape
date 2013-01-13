Escape
======

Escape is a UNIX-like microkernel operating system on which I'm working since
october 2008. It's implemented in ANSI C, C++ and a bit assembler. Except that
I'm using the bootloader GRUB and the libraries that GCC provides (libgcc,
libsupc++), I've developed the whole OS by myself.  
Escape runs on x86, ECO32 and MMIX.
[ECO32](http://homepages.thm.de/~hg53/eco32/) is a 32-bit big-endian RISC
architecture, created by Hellwig Geisse at the University of Applied Sciences
in Gießen for research and teaching purposes.
MMIX is a 64-bit big-endian RISC architecture, developed by Donald Knuth as
the successor of MIX, which is the abstract machine that is used in the famous
bookseries The Art of Computer Programming. More precisely, Escape runs only
on [GIMMIX](http://homepages.thm.de/~hg53/gimmix/), a simulator for MMIX
developed by myself for my master thesis (the differences are minimal, but
currently required for Escape).


General structure
-----------------

Escape consists of a kernel, drivers, libraries and user-applications. The
kernel is responsible for processes, threads, memory-management, a virtual
file system (VFS) and some other things. Drivers run in userspace and work
via message-passing. They do not necessarily access the hardware and should
therefore more seen as an instance that provides a service.  
The VFS is one of the central points of Escape. Besides the opportunity to
create files and folders in it, it is used by the kernel to provide
information about the system for the userspace (memory-usage, CPU, running
processes, ...). Most important, it is used to communicate with devices,
which are registered and handled by drivers. That means, each device gets
a node in the VFS over which it can be accessed. So, for example an
`open("/dev/zero",...)` would open the device "zero". Afterwards one can use
the received file-descriptor to communicate with it.  
Basically, the communication works over messages. Escape provides the
system-calls send and receive for that purpose. Additionally, Escape defines
standard-messages for open,read,write and close, which devices may support.
For example, when using the read system-call with a file-descriptor for a
device, Escape will send the read-standard-message to the corresponding
driver and handle the communication for the user-program. As soon as the
answer is available, the result is passed back to the user-program that
called read. The same mechanism is used for the "real" file-system, which
is realized by the driver "fs". Escape defines messages for that purpose
and handles the communication with fs when a user-program accesses a "real"
file.  
Because of this mechanism the user-space doesn't have to distinguish between
virtual files, real files and devices. So, for example the tool cat can simply
do an open and use read to read from that file and write to write to stdout.


Features
--------

Escape has currently the following features:

* **Kernel**
    * Memory-management supporting copy-on-write, text-sharing,
      shared-libraries, shared-memory, mapping of physical mem (e.g. VESA-mem),
      demand-loading, swapping (not finished yet) and thread-local-storage.
    * Process- and thread-management, event-system, ELF-loader, environment-
      variables, scheduler, signal-management (signals are also used to pass
      interrupts to drivers) and timer-management (for sleeping and
      preemption).
    * SMP-support, CPU-detection and CPU-speed-calculation, FPU-support
      VM86-task to perform BIOS-calls
    * Virtual file system for driver-access, accessing the real-filesystem,
      creating virtual files and directories, using pipes and providing
      information about the system (memory-usage, cpu, running processes, ...)
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
* **libgui**  
  The GUI-library isn't finished yet, but does already provide things like
  basic controls, layout-manager, BMP-support, event-system (keypresses, ...)
  and so on. It is written in C++, which was one of the main reason to provide
  C++-support in Escape.
* **drivers**  
  Escape has some architecture-specific drivers and some common drivers that
  are architecture- independent. Currently, there are:
    * common
        * fs: The filesystem. Has a nearly complete ext2-rev0 fs and a iso9660
          fs. Additionally it manages mount-points.
        * kmmanager: The keymap-manager knows the current keymap (can be
          changed), reads from keyboard, translates the keycodes to characters
          and provides them for other processes.
        * ui: Registers keyboard-shortcuts at kmmanager and allows this way to
          switch the virtual terminal via CTRL+1-6. CTRL+7 switches to the GUI.
        * vterm: An abstraction-layer for user-processes. It reads from
          kmmanager, writes to the video-driver and registers multiple virtual
          terminals as devices. They can be used to read characters or write
          characters (terminal-like).
        * random/null/zero: Like /dev/{random,null,zero} in e.g. Linux
        * winmanager: Manages all existing windows. Reads from kmmanager and
          mouse and passes it to the corresponding application. Additionally it
          can move and resize windows and is responsible for setting the
          corresponding mouse-cursor (default, resize, ...).
    * x86
        * ata: Reading and writing from/to ATA/ATAPI-devices. It supports
          PIO-mode and DMA, LBA28 and LBA48.
        * cmos: Provides the current date.
        * keyboard: Gets notified about keyboard-interrupts, converts the
          scancodes to keycodes and stores them in a ringbuffer. Other
          processes can read them.
        * mouse: Gets notified about mouse-interrupts, stores the information
          in a ring-buffer and gives other processes read-access to them.
        * pci: Collects the available PCI-devices and gives others access to
          them. This is e.g. used by ata to find the IDE-controller.
        * speaker: Produces beeps with the PC-speaker with a specified
          frequency and duration.
        * vesa: Reads the supported VESA-modes from BIOS (via VM86), determines
          the best matching mode and sets it. It creates a shared-memory-region
          which all GUI-processes use directly. On demand it copies a part of
          it to the VESA-memory. Additionally one can let VESA draw the mouse-
          cursor at a specified position and it is also responsible for drawing
          the window-move/resize-preview-rectangle.
        * vesatext: VESA driver for text-mode. Can be used as an alternative to
          the video-driver (VGA-text)
        * video: Can be used to write to the VGA-textmode-memory.
    * ECO32/MMIX  
      Both architectures have the same devices atm (the only difference is that
      ECO32-devices are accessed with 32-bit words and MMIX-devices with 64-bit
      words).
        * disk: the disk-driver and therefore the equivalent to ata on x86.
        * keyboard: the keyboard-driver
        * rtc: the real-time-clock. This is a dummy atm, because both machines
          don't provide a real-time-clock. Therefore, a timestamp is simply
          increased once a second, starting with 0 :)
        * video: the video-driver (text-based)
* **user**  
  There are quite a few user-programs so that you can do something with Escape
  ;) The most important ones are:
    * initloader: Is included in the kernel and is used as the first process.
      Loads the boot-modules (disk-driver, pci (on x86 only), cmos/rtc and fs
      because we need them to load other drivers and user-apps from the disk).
      After that it replaces itself with init.
    * init: Is responsible for loading the drivers and login-shells. Which
      drivers should be loaded is determined by /etc/drivers. It starts one
      login-shell for each vterm.
    * dynlink: the dynamic linker of Escape that is started by the kernel, if
      the requested application is dynamically linked. It receives the fd for
      the executable, loads all dependencies, does the relocation and
      initialization and finally jumps to the application.
    * login: Accepts username+password, sets uid, gid and further group-ids
      using /etc/users and /etc/groups, creates stdin, stdout and stderr, sets
      the env-vars CWD and USER and exchanges itself with the shell.
    * shell: The shell implements a small shell-scripting-language-interpreter
      with pipes, io-redirection, path-expansion, background-jobs, arithmetic,
      loops, if-statements, functions, variables and arrays. The interpreter
      is realized with flex and bison.
    * cat, chmod, chown, cut, date, dd, dump, grep, groups, head, kill, less,
      ln, ls, mkdir, more, mount, ps, readelf, rm, rmdir, sort, stat, sync,
      tail, time, top, umount, users, wc: The well-known UNIX-tools. They
      don't do exactly the same and are, of course, much simpler, but in
      principle they are intended for the same things.
    * ts: analogously to ps, ts lists all threads
    * power: for reboot / shutdown using init.
    * keymap: A tool to change the current keymap (us and german is available)
    * ...


Getting started
---------------

1. At first you need to build the cross-compiler for the desired
   architecture:  
   `$ cd cross`  
   `$ ./build.sh (i586|eco32|mmix)`  
   This will download gcc and binutils, build the cross-compiler and put it
   into /opt/escape-cross-$arch.
2. Now you can build escape:  
   `$ cd ../source`  
   `$ export ESC_TARGET=$arch` # choose $arch appropriately  
   `$ ./b`
3. To start it, you have to choose a boot-script at boot/$ESC_TARGET/.
   E.g.:  
   `$ ./b run qemu-cd`
4. For further information, take a look at `./b -h`

