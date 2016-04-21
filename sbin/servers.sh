# Run a shell command on all servers.
#
# Environment Variables
#
#   SUCCINCT_SERVERS    File naming remote servers.
#     Default is ${SUCCINCT_CONF_DIR}/servers.
#   SUCCINCT_CONF_DIR  Alternate conf dir. Default is ${SUCCINCT_HOME}/conf.
#   SUCCINCT_HOST_SLEEP Seconds to sleep between spawning remote commands.
#   SUCCINCT_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: servers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# If the servers file is specified in the command line,
# then it takes precedence over the definition in
# succinct-env.sh. Save it here.
if [ -f "$SUCCINCT_SERVERS" ]; then
  SERVERLIST=`cat "$SUCCINCT_SERVERS"`
fi

# Check if --config is passed as an argument. It is an optional parameter.
# Exit if the argument is not a directory.
if [ "$1" == "--config" ]
then
  shift
  conf_dir="$1"
  if [ ! -d "$conf_dir" ]
  then
    echo "ERROR : $conf_dir is not a directory"
    echo $usage
    exit 1
  else
    export SUCCINCT_CONF_DIR="$conf_dir"
  fi
  shift
fi

. "$SUCCINCT_PREFIX/sbin/load-succinct-env.sh"

if [ "$SERVERLIST" = "" ]; then
  if [ "$SUCCINCT_SERVERS" = "" ]; then
    if [ -f "${SUCCINCT_CONF_DIR}/servers" ]; then
      SERVERLIST=`cat "${SUCCINCT_CONF_DIR}/servers"`
    else
      SERVERLIST=localhost
    fi
  else
    SERVERLIST=`cat "${SUCCINCT_SERVERS}"`
  fi
fi



# By default disable strict host key checking
if [ "$SUCCINCT_SSH_OPTS" = "" ]; then
  SUCCINCT_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for host in `echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${SUCCINCT_SSH_FOREGROUND}" ]; then
    ssh $SUCCINCT_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $SUCCINCT_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$SUCCINCT_HOST_SLEEP" != "" ]; then
    sleep $SUCCINCT_HOST_SLEEP
  fi
done

wait
