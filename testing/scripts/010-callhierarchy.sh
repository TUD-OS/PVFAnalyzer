#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/callhierarchy.bin -o $TESTOUT/callhierarchy.cfg -e 0x16
$ROOTDIR/build/printer/printer -f $TESTOUT/callhierarchy.cfg -ccall -o -
