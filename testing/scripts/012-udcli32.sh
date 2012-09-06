#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/udcli32.bin \
		-o $TESTOUT/udcli32.cfg -e 0x80487cc \
		-t 0x8048a8c,0x8048b63,0x8048c0d,0x8048d09,0x8048d4b,0x8048d98,0x8048dde,0x8048e39,0x8049036
$ROOTDIR/build/printer/printer -f $TESTOUT/udcli32.cfg -cfunc -o -
