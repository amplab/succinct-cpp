#!/usr/bin/env bash

pids="`pgrep aserver`"

for pid in $pids
do
	echo "Killing pid $pid..."
	kill -9 $pid
done
