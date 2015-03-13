# Starts an adaptive-handler and an adaptive shard on this node

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# Start Servers
echo "Starting adaptive server..."
"$sbin"/start-aserver.sh $1 0

# Wait for some time
sleep 60

# Start Handlers
echo "Starting adaptive handler..."
"$sbin"/start-ahandler.sh 1 0
