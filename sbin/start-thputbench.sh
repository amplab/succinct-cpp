#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# If the hosts file is specified in the command line,
# then it takes precedence over the definition in
# succinct-env.sh. Save it here.
if [ -f "$SUCCINCT_HOSTS" ]; then
  HOSTLIST=`cat "$SUCCINCT_HOSTS"`
fi

. "$SUCCINCT_PREFIX/bin/load-succinct-env.sh"

if [ "$SUCCINCT_RES_PATH" = "" ]; then
	SUCCINCT_RES_PATH="$SUCCINCT_HOME/res"
fi

mkdir -p $SUCCINCT_RES_PATH

"$sbin/hosts.sh" cd "$SUCCINCT_HOME" \; "$sbin/start-thputbench-local.sh" "$@"
"$sbin/hosts.sh" cd "$SUCCINCT_HOME" \; awk '{ sum += \$1 } END { print sum }' throughput_results_access > "$SUCCINCT_RES_PATH/thput"
"$sbin/hosts.sh" cd "$SUCCINCT_HOME" \; rm throughput_results_access