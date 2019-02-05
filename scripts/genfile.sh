#!/bin/bash
rm input.txt
for ((i=0;i<$1;i+=1)); do
    for ((k=0;k<$2;k+=1)); do
        echo -n "A" >> input.txt;
    done
    for ((k=0;k<$2;k+=1)); do
        echo -n "B" >> input.txt;
    done
    for ((k=0;k<$2;k+=1)); do
        echo -n "C" >> input.txt;
    done
done
