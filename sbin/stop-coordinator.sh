#!/usr/bin/env bash

pids="`pgrep scoordinator`"

for pid in $pids
do
	echo "Killing pid $pid..."
	kill -9 $pid
done
