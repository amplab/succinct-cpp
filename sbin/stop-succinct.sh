#!/usr/bin/env bash

# Stops all servers and the aggregator

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

# Stop servers
echo "Stopping servers..."
"$sbin/stop-servers.sh"

# Stop aggregator
echo "Stopping aggregator..."
"$sbin/stop-aggregator.sh"
