#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/analysis/qsort_inline -o $TESTOUT/qsort_it.cfg -e 0x8048572
$ROOTDIR/build/dynrun/dynrun -f $TESTOUT/qsort_it.cfg -o $TESTOUT/qsort_it.ilist -- $TESTCASES/analysis/qsort_inline
$ROOTDIR/build/pvfregs/pvfregs -f $TESTOUT/qsort_it.ilist
