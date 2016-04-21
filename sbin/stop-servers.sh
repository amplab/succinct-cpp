#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

exec "$sbin/servers.sh" "$SUCCINCT_PREFIX/sbin/stop-server.sh"
