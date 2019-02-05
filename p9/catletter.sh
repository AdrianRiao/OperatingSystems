#!/bin/sh

cd $1

for x in $(ls -1 *.txt | tr A-Z a-z | sort | cut -c 1)
do
	if test -f $(echo $x.output) 
	then
		rm $x.output
	fi
done

for x in $(ls -1 *.txt | sort)
do
	cat $x >> $(echo $x | cut -c 1 | tr A-Z a-z).output
done
exit 0
