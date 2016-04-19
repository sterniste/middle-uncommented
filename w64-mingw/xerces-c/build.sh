#!/bin/bash
XERCESC_SRC_DIR=$1
if [ ! -d "$XERCESC_SRC_DIR" ]; then
    echo "can't find source dir: $XERCESC_SRC_DIR"
    exit 1
fi

TARGET_DIR=`pwd`/../target
cd $XERCESC_SRC_DIR
./configure --host=x86_64-w64-mingw32 --prefix=$TARGET_DIR --disable-shared --disable-samples --disable-tests
make clean
make VERBOSE=1 LDFLAGS=-no-undefined && make install
