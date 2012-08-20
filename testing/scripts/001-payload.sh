#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/payload.bin -o $TESTOUT/payload.cfg
$ROOTDIR/build/printer/printer -f $TESTOUT/payload.cfg -o -
