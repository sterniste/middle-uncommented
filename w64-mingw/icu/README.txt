Building ICU
============

Download the ICU4C sources (http://site.icu-project.org/download/, I used version 55),
unpack them into some directory somewhere (just underneath this directory, for instance),
then run the command:

$ bash build.sh srcdir

Where srcdir is the directory in which the ICU4C sources were just unpacked.

Afterwards, verify that the ICU static libraries have been installed in ../target/lib
and the ICU header files in ../target/include.

When building under Linux, the build fails during the "install documentation" phase for
some reason. However, at that point the libraries have already been successfully built.
