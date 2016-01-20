include "heartbeat.thrift"

namespace cpp succinct

service Handler {
	// Connect to/Disconnect from all other handlers
	i32 ConnectToHandlers(),
	i32 DisconnectFromHandlers(),

	// Connect to/Disconnect from local servers
	i32 ConnectToLocalServers(),
	i32 DisconnectFromLocalServers(),
		
	// Start local servers
	i32 StartLocalServers(),
		
	// Supported operations
	string Get(1:i64 key),
	string GetLocal(1:i32 qserver_id, 2:i64 key),
		
	set<i64> Search(1:string query),
	set<i64> SearchLocal(1:string query),

	// I-Am-Alive messages to master
	heartbeat.HeartBeat GetHeartBeat(),
}