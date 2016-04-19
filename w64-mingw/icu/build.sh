#!/bin/bash
ICU_REL_DIR=$1
if [ ! -d "$ICU_REL_DIR" -o ! -d "$ICU_REL_DIR"/source ]; then
    echo "can't find release dir: $ICU_REL_DIR"
    exit 1
fi

BUILD_ROOT=`pwd`
TARGET_DIR=`pwd`/../target

NATIVE_TARGET_DIR=`pwd`/native-target
if [ -d $NATIVE_TARGET_DIR ]; then
   rm -rf $NATIVE_TARGET_DIR
fi
mkdir $NATIVE_TARGET_DIR
NATIVE_BUILD_DIR=`pwd`/native-build
if [ -d $NATIVE_BUILD_DIR ]; then
   rm -rf $NATIVE_BUILD_DIR
fi
cp -r $ICU_REL_DIR/source $NATIVE_BUILD_DIR
cd $NATIVE_BUILD_DIR
./configure --prefix=$NATIVE_TARGET_DIR --disable-samples --disable-tests
make clean
make VERBOSE=1 && make install

cd $BUILD_ROOT

CROSS_BUILD_DIR=`pwd`/cross-build
if [ -d $CROSS_BUILD_DIR ]; then
   rm -rf $CROSS_BUILD_DIR
fi
cp -r $ICU_REL_DIR/source $CROSS_BUILD_DIR
cd $CROSS_BUILD_DIR
./configure --host=x86_64-w64-mingw32 --with-cross-build=$NATIVE_BUILD_DIR --prefix=$TARGET_DIR --disable-shared --enable-static --disable-samples --disable-tests --disable-tools
make clean
make VERBOSE=1 LDFLAGS=-L$NATIVE_TARGET_DIR/lib && make install
