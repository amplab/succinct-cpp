# Starts an aggregator and $NUM_SHARDS servers

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/succinct-config.sh"

# Check if the binaries exist
if [ ! -d "$SHARDED_KV_BIN_DIR" ]; then
    echo "Must build Succinct before starting service."
    exit -1
fi

# Set paths to binaries
export QUERY_SERVER="${SHARDED_KV_BIN_DIR}/skvserver"
export AGGREGATOR="${SHARDED_KV_BIN_DIR}/skvaggregator"
export INITIALIZER="${SHARDED_KV_BIN_DIR}/skvinitializer"

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
