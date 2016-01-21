#!/usr/bin/env bash

pids="`pgrep succinct-handler`"

for pid in $pids
do
	echo "Killing pid $pid..."
	kill -9 $pid
done
