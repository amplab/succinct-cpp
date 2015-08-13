# Starts an aggregator and $NUM_SHARDS servers

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# Start Servers
echo "Starting servers..."
"$sbin"/start-servers.sh

# Wait for some time
sleep 1

# Start Aggregator
echo "Starting aggregator..."
"$sbin"/start-aggregator.sh
