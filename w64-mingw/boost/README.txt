Building Boost
==============

Download the boost sources (http://www.boost.org/users/download/, I used version 1.59),
unpack them into some directory somewhere (just underneath this directory, for instance),
then run the command:

$ bash build.sh srcdir

Where srcdir is the directory in which the boost sources were just unpacked.

Afterwards, verify that the boost static libraries have been installed in ../target/lib
and the boost header files in ../target/include.

