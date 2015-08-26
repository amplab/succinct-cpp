#!/usr/bin/env bash

pids="`pgrep ${QUERY_SERVER_PROCESS}`"

for pid in $pids
do
	echo "Killing pid $pid..."
	kill -9 $pid
done
