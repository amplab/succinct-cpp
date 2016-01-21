# Usage: start-handler.sh <handler-id>
usage="Usage: start-handler.sh <handler-id>"

if [ $# -ne 1 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$HANDLER_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$LOG_PATH" = "" ]; then
	LOG_PATH="$SUCCINCT_HOME/log/"
fi

nohup "$bin/succinct-handler" "$1"&
