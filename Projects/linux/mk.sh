#!/bin/sh
#make clean && make
if [ $# -eq 0 ]; then
    echo "Usage:./mk.sh [platform: x86|mips32|jz4775] [version:debug|release]"
    exit 0;
fi

if [ $1 == "clean" ]; then
    make clean;
    exit 0;
fi

case "$1" in
    x86) echo "Build default";
        export PRODUCT=x86;
        ;;
    mips32) echo "Build mips32";
        export PRODUCT=mips32;
        ;;
    junzheng) echo "build junzheng";
        export PRODUCT=junzheng;
        ;;
esac
case "$2" in
    debug) echo "connect to china.xtremeprog.com";
        export VERSION_TYPE=DEBUG;
        ;;
    release) echo "connect to site";
        export VERSION_TYPE=RELEASE;
        ;;
esac
make clean && make -f Makefile
