A small guide to build the cross-compiler:

1. At first you need binutils-2.20, gcc-core-4.4.3, gcc-g++-4.4.3 and newlib-1.15.0. If you
   ask yourself why newlib is needed: Its just to prevent problems during build. In the end
   its not used at all since escape has its own c-standard-library.
   You can download the 4 archives by executing ./download.sh. It will put them in the current
   directory, i.e. <your escape folder>/cross.
2. Now simply execute ./build.sh. It extracts all to the subdirectory "src" and puts the build-
   files in ../build/cross. The files needed to produce executables for escape and so on are put
   in ../build/dist.
   It builds binutils, gcc, libgcc, newlib and finally libsupc++ (for which newlib is needed to
   fix the build).
3. That's it :)

If you want to rebuild it completely, use ./build.sh --rebuild. To refresh something, use just
./build.sh.