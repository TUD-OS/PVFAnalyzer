#!/bin/bash

PLT=cmd.plt
REGISTERS="EAX EBX ECX EDX ESP EBP"

set -x
echo "set yrange [0:1.3]" >$PLT
echo "set xlabel 'Time [instruction blocks]'" >>$PLT
echo "set ylabel 'Raw PVF'" >> $PLT
echo "plot \\" >>$PLT

for reg in $REGISTERS; do
	cat log.00 | grep $reg | cut -d= -f 2 > $reg
	cat log.01 | grep $reg | cut -d= -f 2 >>$reg
	cat log.02 | grep $reg | cut -d= -f 2 >>$reg
	cat log.03 | grep $reg | cut -d= -f 2 >>$reg
done

for reg in EAX EBX ECX EDX ESP; do
	echo "'./$reg' u 0:1 w p, \\" >>$PLT
done
echo "'./EBP' u 0:1 w p" >>$PLT

echo "pause -1" >>$PLT
gnuplot $PLT
