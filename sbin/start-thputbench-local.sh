# Usage: start-thputbench-local.sh [-t <#threads>] [-s <#shards>] [-k <#keys>]

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$BENCH_BIN_DIR"
bin="`cd "$bin"; pwd`"

"$bin/ssbench" "$@" 2>&1 &
