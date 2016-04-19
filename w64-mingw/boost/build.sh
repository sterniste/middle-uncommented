#!/bin/bash
BOOST_SRC_DIR=$1
if [ ! -d "$BOOST_SRC_DIR" ]; then
    echo "can't find source dir: $BOOST_SRC_DIR"
    exit 1
fi

export TARGET_DIR=`pwd`/../target
export BUILD_DIR=`pwd`/build
if [ -d $BUILD_DIR ]; then
   rm -rf $BUILD_DIR
fi
mkdir $BUILD_DIR   
export BOOST_BUILD_USER_CONFIG=`pwd`/user-config.jam
cd $BOOST_SRC_DIR
./bootstrap.sh --without-icu mingw
if [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ]; then
    echo 'using gcc : 4.9 : x86_64-w64-mingw32-g++.exe ;' > $BOOST_BUILD_USER_CONFIG
    ./bjam.exe --debug-config --build-dir=$BUILD_DIR toolset=gcc target-os=windows variant=release --prefix=$TARGET_DIR threading=multi threadapi=win32 link=static runtime-link=static address-model=64 --layout=tagged --with-log --with-program_options --with-system --with-thread -j2 stage install
else
    echo 'using gcc : 4.9 : x86_64-w64-mingw32-g++ ;' > $BOOST_BUILD_USER_CONFIG
    ./bjam --debug-config --build-dir=$BUILD_DIR toolset=gcc target-os=windows variant=release --prefix=$TARGET_DIR threading=multi threadapi=win32 link=static runtime-link=static address-model=64 --layout=tagged --with-log --with-program_options --with-system --with-thread -j2 stage install
fi
ln -s $TARGET_DIR/lib/libboost_thread_win32-mt-s.a $TARGET_DIR/lib/libboost_thread-mt-s.a
