#!/bin/sh
for f in *.h *.cpp; do echo -n $f; head -2 $f | tail -1; done
