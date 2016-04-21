#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

if [ "$SUCCINCT_RES_PATH" = "" ]; then
	SUCCINCT_RES_PATH="$SUCCINCT_HOME/res"
fi

mkdir -p $SUCCINCT_RES_PATH

"$sbin/servers.sh" cd "$SUCCINCT_HOME" \; "$sbin/start-thputbench-local.sh" "$@"
"$sbin/servers.sh" cd "$SUCCINCT_HOME" \; awk '{ sum += \$1 } END { print sum }' thput > "$SUCCINCT_RES_PATH/thput"
"$sbin/servers.sh" cd "$SUCCINCT_HOME" \; rm thput
