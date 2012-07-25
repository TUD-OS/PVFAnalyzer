#!/bin/sh
for f in $(find . -name *.h) $(find . -name *.cpp); do
		echo -n $f; head -2 $f | tail -1 | grep -v "\^\./build";
done
