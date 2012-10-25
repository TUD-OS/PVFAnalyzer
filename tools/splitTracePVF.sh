#!/bin/sh

if [ -z "$1" ]; then
  echo "Usage: $0 <binary> <tracefile>"
  exit 2
fi

if [ -z "$2" ]; then
  echo "Usage: $0 <binary> <tracefile>"
  exit 2
fi

BINARY=$1
TRACEFILE=$2
STEPS=1000

ENTRY=$(nm $BINARY | grep " main$" | cut -d\  -f 1)

echo "==== CFG Builder ==="
build/reader/reader -f $BINARY -o $BINARY.cfg -e 0x$ENTRY

echo "==== Splitting trace ($STEPS) ==="
split -a 5 -n l/$STEPS -d $TRACEFILE tmp/trace-

echo "==== Off we go..."
TRACELIST=$(ls tmp/trace-*);
for f in $TRACELIST; do 
	build/unroll/unroll -f $BINARY.cfg -t $f -o $f.ilist
	tools/pvfrun $f.ilist
done

rm $BINARY.cfg $TRACELIST tmp/*.ilist
