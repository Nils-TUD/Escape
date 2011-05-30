A small guide to build and use escape:

1. At first you need to download and build the cross-compiler:
   $ cd cross
   $ ./download.sh
   $ ./build.sh
	
   The download-script downloads binutils-2.20, gcc-core-4.4.3, gcc-g++-4.4.3 and newlib-1.15.0 into
   the current directory. The build-script extracts the archives to src, patches some files and
   puts the build-files in ../build/cross. The files needed to produce executables for escape and
   so on are put in ../build/dist.
   If you ask yourself why newlib is needed: Its just to prevent problems during build. In the end
   its not used at all since escape has its own c-library.
   If you want to rebuild the cross-compiler completely, use ./build.sh --rebuild. To refresh
   something, use just ./build.sh.

2. Now you can build escape via:
   $ cd ..
   $ make

3. To run it use "make qemu", "make bochs", "make vbox", "make vmware" or similar. Please take
   a look at Makefile to see what commands exist and if you have to change settings (which you
   probably have to do).
