#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/bin/load-succinct-env.sh"

exec "$sbin/hosts.sh" "$SUCCINCT_PREFIX/sbin/stop-aworker.sh"
