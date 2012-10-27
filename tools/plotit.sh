#!/bin/bash

# preparation:
#
# 1) I split the input EIP trace into 4 parts:
#    $> split -d -n l/4 trace.O3.txt traceO3-
#
# 2) Run 4 instances of splitPVFTrace:
#    $> tools/splitTracePVF.sh system.O3.elf traceO3-00 | tee log.00
#    $> tools/splitTracePVF.sh system.O3.elf traceO3-01 | tee log.01
#    $> tools/splitTracePVF.sh system.O3.elf traceO3-02 | tee log.02
#    $> tools/splitTracePVF.sh system.O3.elf traceO3-03 | tee log.03
#
# 3) Plot the data using this script

PLT=cmd.plt
REGISTERS="EAX ECX EDX EBX ESP EBP"

set -x
echo "set yrange [0:1.3]" >$PLT
echo "set terminal png size 1024,768" >> $PLT
echo "set output 'pvf.png'" >>$PLT
echo "set xlabel 'Time [instruction blocks]'" >>$PLT
echo "set ylabel 'Raw PVF'" >> $PLT
echo "plot \\" >>$PLT

for reg in $REGISTERS; do
	cat log.00 | grep $reg | cut -d= -f 2 > $reg
	cat log.01 | grep $reg | cut -d= -f 2 >>$reg
	cat log.02 | grep $reg | cut -d= -f 2 >>$reg
	cat log.03 | grep $reg | cut -d= -f 2 >>$reg
done

for reg in EAX ECX EDX EBX ESP; do
	echo "'./$reg' u 0:1 w p title '$reg', \\" >>$PLT
done
echo "'./EBP' u 0:1 w p title 'EBP'" >>$PLT

echo "pause -1" >>$PLT
gnuplot $PLT
