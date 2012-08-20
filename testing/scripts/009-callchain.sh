#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/callchain.bin -o $TESTOUT/callchain.cfg -e 0x2d
$ROOTDIR/build/printer/printer -f $TESTOUT/callchain.cfg -c -o -
