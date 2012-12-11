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
LINES=10000

ENTRY=$(nm $BINARY | grep " main$" | cut -d\  -f 1)

# don't need CFG builder as we use failTrace below
#echo "==== CFG Builder ==="
#build/reader/reader -f $BINARY -o $BINARY.cfg -e 0x$ENTRY

echo "==== Splitting trace ($LINES) ==="
split -a 5 -l 10000 -d $TRACEFILE tmp/trace-$TRACEFILE-

echo "==== Off we go..."
TRACELIST=$(ls tmp/trace-$TRACEFILE-*);
for f in $TRACELIST; do 
	build/failTrace/failTrace -f $BINARY -t $f -o $f.ilist
	tools/pvfrun $f.ilist
done

#rm $TRACELIST tmp/*.ilist
