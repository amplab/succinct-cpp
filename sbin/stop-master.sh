#!/usr/bin/env bash

pids="`pgrep succinct-master`"

for pid in $pids
do
	echo "Killing pid $pid..."
	kill -9 $pid
done
