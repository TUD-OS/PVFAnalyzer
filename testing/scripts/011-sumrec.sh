#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/sum_rec.bin -o $TESTOUT/sum_rec.cfg -e 0x8048400
$ROOTDIR/build/printer/printer -f $TESTOUT/sum_rec.cfg -ccall -o -
