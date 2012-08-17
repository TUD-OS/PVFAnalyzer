#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/recursion.bin -o $TESTOUT/recursion.cfg -e 0x5
$ROOTDIR/build/printer/printer -f $TESTOUT/recursion.cfg -o -
