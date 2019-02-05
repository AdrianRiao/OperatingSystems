#!/bin/sh

Nciu=$(egrep -v -e '^$' -e '^#' $1 | egrep -e '^[a-zA-Z]' | wc -l)

for i in $(seq 1 $Nciu)
do
	ciu=$(egrep -v -e '^$' -e '^#' $1 |
	      egrep -e '^[a-zA-Z]' |
	      sed -n $i'p')

        prof=$(egrep -v -e '^$' -e '^#' $1 |
               sed -E -n '/'"$ciu"'/,/[a-zA-Z]/p' |
               awk '/[0-9]/{ print $3 }' |
               awk '{ SUM+=$1 } END { print SUM/NR}')

        mag=$(egrep -v -e '^$' -e '^#' $1 |
              sed -E -n '/'"$ciu"'/,/[a-zA-Z]/p' | 
              awk '/[0-9]/{ print $4 }' |
              sort -g | sed -n '$p')

	echo $(echo $ciu | tr A-z a-z) $prof $mag
done
exit 0 
