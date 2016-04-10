#!/bin/sh

# run regression tests on interpreter

PROG=../tstest
ok="ok"
endmsg=$ok

#
# run regression tests
#
for i in *.ts
do
    j=`basename $i .ts`
    echo $i ":" $j
    $PROG $i > $j.txt
    if diff -ub $j.expect $j.txt
    then
	echo $j passed
	rm -f $j.txt
    else
	echo $j failed
	endmsg="TEST FAILURES"
    fi
done
echo $endmsg
