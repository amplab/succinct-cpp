include "heartbeat.thrift"

namespace cpp succinct

service Server {
	// Connect to/Disconnect from all other handlers
	i32 ConnectToServers(),
	i32 DisconnectFromServers(),
		
	// Supported operations
	string Get(1:i64 key),
	string GetLocal(1:i32 qserver_id, 2:i64 key),
		
	list<i64> Search(1:string query),
	list<i64> SearchLocal(1:string query),
}
