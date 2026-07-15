#!/bin/bash

max=64
n=1

while [ $n -le $max ]
do
    echo "Running with $n processes"
    mpiexec -n $n -f machinefile /home/bxjs/2023150001/a.o
    echo "-----------------------------------"

    n=$((n*2))
done