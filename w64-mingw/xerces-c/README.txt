Building Xerces-C
=================

Download the boost sources (http://xerces.apache.org/xerces-c/download.cgi, I used version 3.1.2),
unpack them into some directory somewhere (just underneath this directory, for instance),
then run the command:

$ bash build.sh srcdir

Where srcdir is the directory in which the Xerces-C sources were just unpacked.

Afterwards, verify that the Xerces-C static libraries have been installed in ../target/lib
and the Xerces-C header files in ../target/include.

