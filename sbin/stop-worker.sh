#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

"$sbin/stop-handler.sh"
"$sbin/stop-servers.sh"