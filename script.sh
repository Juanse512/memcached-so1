#!/bin/bash
K=$1
N=0
while [ $N -lt 1000 ]; do
printf "PUT $K $N\n"
printf "GET $K\n"
N=$((N+1))
K=$((K+1))
done | nc localhost 888
