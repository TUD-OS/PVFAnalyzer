#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/hello_dynamic.bin -o $TESTOUT/hello_dynamic.cfg -e 0x804841c
$ROOTDIR/build/printer/printer -f $TESTOUT/hello_dynamic.cfg -ccall -o -
