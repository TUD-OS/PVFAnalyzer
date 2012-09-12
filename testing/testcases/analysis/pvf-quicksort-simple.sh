#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/analysis/qsort_simple -o $TESTOUT/qsort_ret.cfg -e 0x080484ec
$ROOTDIR/build/dynrun/dynrun -f $TESTOUT/qsort_ret.cfg -o $TESTOUT/qsort_ret.ilist -- $TESTCASES/analysis/qsort_simple
$ROOTDIR/build/pvfregs/pvfregs -f $TESTOUT/qsort_ret.ilist
