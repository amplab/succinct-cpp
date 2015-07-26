# Usage: start-thputbench-local.sh [-t <#threads>] [-s <#shards>] [-k <#keys>]

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$THRIFT_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

"$bin/ssbench" "$@" 2>&1 &
