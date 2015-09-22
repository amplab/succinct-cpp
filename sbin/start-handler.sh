# Usage: start-handler.sh <#shards> <handler#>
usage="Usage: start-handler.sh <#shards> <handler#>"

if [ $# -le 1 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

bin="$THRIFT_BIN_DIR"
bin="`cd "$bin"; pwd`"

export LD_LIBRARY_PATH=$SUCCINCT_HOME/lib

if [ "$SUCCINCT_LOG_PATH" = "" ]; then
	SUCCINCT_LOG_PATH="$SUCCINCT_HOME/log"
fi

if [ "$SUCCINCT_HOSTS" = "" ]; then
	SUCCINCT_HOSTS="$SUCCINCT_CONF_DIR/hosts"
fi

if [ "$BLOWFISH_CONF" = "" ]; then
	if [ -f "${SUCCINCT_CONF_DIR}/blowfish.conf" ]; then
		BLOWFISH_CONF="${SUCCINCT_CONF_DIR}/blowfish.conf"
	else
		echo "Conf file not found"
		exit
	fi
fi

mkdir -p $SUCCINCT_LOG_PATH

nohup "$bin/shandler" -s "${1}" -h "$SUCCINCT_HOSTS" -c "$BLOWFISH_CONF" -i "${2}" 2>"$SUCCINCT_LOG_PATH/handler_${2}.log" &
