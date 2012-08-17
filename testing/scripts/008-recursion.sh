#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/cfg_reader/cfg -f $TESTCASES/recursion.bin -o $TESTOUT/recursion.cfg -e 0x5
$ROOTDIR/build/cfg_printer/cfg_printer -f $TESTOUT/recursion.cfg -o -
