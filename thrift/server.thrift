include "heartbeat.thrift"

namespace cpp succinct

service Server {
	// Intialize
	i32 Intialize(1:i32 shard_id, 2:i32 replica_id),
	
	// Supported operations
	string Get(1: i64 key),
	set<i64> Search(1: string query),

	// I-Am-Alive messages to handler
	heartbeat.HeartBeat GetHeartBeat(),
}
