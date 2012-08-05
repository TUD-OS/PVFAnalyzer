#!/bin/sh
DIRS="analyzer common testing"
for d in $DIRS; do
	for f in $(find $d -name *.h) $(find $d -name *.cpp); do
			echo -n $f; head -2 $f | tail -1;
	done
done
