#!/bin/bash
N=0
while [ $N -lt 1000 ]; do
./bin_client GET "191"
N=$((N+1))
done
