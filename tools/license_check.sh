#!/bin/sh
DIRS="reader printer common testing unroll dynrun"
for d in $DIRS; do
	for f in $(find $d -name *.h) $(find $d -name *.cpp); do
			echo -n $f; head -2 $f | tail -1;
	done
done
