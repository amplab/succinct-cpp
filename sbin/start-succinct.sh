# Starts an aggregator and $NUM_SHARDS servers

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

if [ ! -d "$SHARDED_BIN_DIR" ]; then
    echo "Must build Succinct before starting service."
    exit -1
fi

# Start Servers
echo "Starting servers..."
"$sbin"/start-servers.sh

# Wait for some time
sleep 1

# Start Aggregator
echo "Starting aggregator..."
"$sbin"/start-aggregator.sh

# Wait for some time
sleep 1

# Start Initializer
echo "Initializing..."
"$sbin"/start-initializer.sh
