include "heartbeat.thrift"

namespace cpp succinct

service Server {
	// Initialize
	i32 Initialize(),
	
	// Supported operations
	string Get(1: i64 key),
	set<i64> Search(1: string query),

	// I-Am-Alive messages to handler
	heartbeat.HeartBeat GetHeartBeat(),
}
