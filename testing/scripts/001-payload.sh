#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/cfg_reader/cfg -f $TESTCASES/payload.bin -o $TESTOUT/payload.cfg
$ROOTDIR/build/cfg_printer/cfg_printer -f $TESTOUT/payload.cfg -o -
