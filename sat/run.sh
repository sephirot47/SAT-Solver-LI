#!/bin/bash
#qmake && make clean && make
if [ ! $? == 0 ]; then
	exit -1
fi

for f in inputs/vars*
do
	echo -e "$f $(./sat < $f)"
done
