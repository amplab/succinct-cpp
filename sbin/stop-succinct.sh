#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

# Stop master
echo "Stopping coordinator..."
"$sbin/stop-coordinator.sh"

# Stop servers
echo "Stopping servers..."
"$sbin/stop-servers.sh"
