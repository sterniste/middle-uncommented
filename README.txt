Building middle
==============

We build middle either for Linux (64 bit) or Windows MinGW (64 bit) target environments.


Building middle under Windows
=============================

On Windows, we need Cygwin as a build environment: the build to produce a MinGW static target (without Cygwin dependencies) is thus a cross build Cygwin -> x86_64-w64-mingw32.

The Cygwin packages cmake, pkg-config, make, and g++ (x86_64-w64-mingw32) must be installed.

First, the 3 third-party libraries Boost, ICU, and Xerces-C are built in folder w64-mingw using gcc/g++ (x86_64-w64-mingw32) with build logic provided by those libraries' maintainers.
In each of the three subdirectories (boost, icu, xerces-c) a README.txt and a BASH shell script build.sh define the process.

Each sub build places it's products in the folder w64-mingw/target: the subsequent cmake build of the middle sources will look there for header files and static libraries.

Before beginning the cmake build, look at the top-level build commands defined in .bash_cmake.
Adjust the MIDDLE_SRC_DIR environment variable there to point to the base of the middle sources (the directory containing both .bash_cmake and w64-mingw).
The shell command:

$ source .bash_cmake

will make the command cmake_release available to your (BASH) shell.

It is advisable to build "out-of-source", in a new build folder (the base directory and its source subdirectories will be unaffected by the build):

$ mkdir build
$ cd build
$ cmake_release ..
$ make

The third command above should produce much output and terminate within seconds. It simply prepares the make build (on the next line) which will take many minutes to complete.
Upon success: the new middle executable can be found in build/main.


Building middle under Linux
===========================

On Linux we can either build native or (as under Cygwin) cross build to produce a x86_64-w64-mingw32 product.

Cross-build for x86_64-w64-mingw32
==================================

For the latter, substitute Linux for Cygwin in the procedure above (Building middle under Windows), installing the same packages.

The .bash_cmake script will choose the wrong (native Linux) build if it finds that it is being run in a non-Cygwin environment.
Thus, to cross build for x86_64-w64-mingw32 under Linux, it will be necessary to sightly rewrite that script or to manually extract the definition of the cmake_release command
so that the cmake_release command has the same definition in the builder's (BASH) shell as it would have under Cygwin.

Native Linux build
==================

Otherwise, if a native Linux build is desired, g++ (x86_64-w64-mingw32) can be dropped from the packages needing installation: install a recent native plain g++ package instead.
The third-party library dependencies (on boost, icu, xerces-c) must also be installed as packages:

libboost-all-dev
libicu-dev
libxerces-c-dev

Thus no preliminary build phase is needed to build/install these libraries locally. Otherwise the procedure is the same as for the cross build.
Use the appropriate definition of cmake_release (not the Cygwin version).
