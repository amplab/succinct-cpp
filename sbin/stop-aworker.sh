#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

"$sbin/stop-ahandler.sh"
"$sbin/stop-aservers.sh"
