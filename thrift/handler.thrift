struct HandlerHeartBeat {
	1: i64 timestamp,
}

service SuccinctService {
	i32 ConnectToHandlers(),
	i32 DisconnectFromHandlers(),

	i32 ConnectToLocalServers(),
	i32 DisconnectFromLocalServers(),

	i32 StartLocalServers(),
	i32 Initialize(1:i32 mode),

	string Get(1:i64 key),
	string GetLocal(1:i32 qserver_id, 2:i64 key),

	string Extract(1:i64 key, 2:i32 offset, 3:i32 length),
	string ExtractLocal(1:i32 qserver_id, 2:i64 key, 3:i32 offset, 4:i32 length),

	set<i64> Search(1:string query),
	set<i64> SearchLocal(1:string query),
    
	set<i64> RegexSearch(1:string query),
	set<i64> RegexSearchLocal(1:string query),

	i64 Count(1:string query),
	i64 CountLocal(1:string query),
	
	HandlerHeartBeat Ping(),
}