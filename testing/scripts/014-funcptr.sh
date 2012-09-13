#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/funcPtr.bin -o $TESTOUT/funcPtr.cfg -e 0x080483e6
$ROOTDIR/build/printer/printer -f $TESTOUT/funcPtr.cfg -ccall -o -
$ROOTDIR/build/dynrun/dynrun -f $TESTOUT/funcPtr.cfg -o $TESTOUT/funcPtr.ilist -q -- $TESTCASES/cfgbuilding/funcPtr.bin
$ROOTDIR/build/printer/printer -f $TESTOUT/funcPtr.cfg -ccall -o -
$ROOTDIR/build/pvfregs/pvfregs -f $TESTOUT/funcPtr.ilist
