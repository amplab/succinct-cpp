#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

# Stop master
echo "Stopping master..."
"$sbin/stop-amaster.sh"

# Stop workers
echo "Stopping workers..."
"$sbin/stop-aworkers.sh"
