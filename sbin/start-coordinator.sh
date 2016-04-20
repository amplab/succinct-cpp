# Starts the coordinator on the host this script is executed on.

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$COORDINATOR_BIN_DIR"
bin="`cd "$bin"; pwd`"

if [ "$LOG_PATH" = "" ]; then
  export LOG_PATH="$SUCCINCT_HOME/log"
fi

mkdir -p $LOG_PATH/stderr

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

nohup "$bin/scoordinator" >/dev/null 2>"$LOG_PATH/stderr/coordinator.stderr" &
