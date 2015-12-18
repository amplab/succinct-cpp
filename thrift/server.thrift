struct ServerHeartBeat {
	1: i64 timestamp,
}

service QueryService {
	i32 Initialize(1:i32 id),
	string Get(1:i64 key),
	string Extract(1:i64 key, 2:i32 offset, 3:i32 length),
	set<i64> Search(1:string query),
	i64 Count(1:string query),
  set<i64> RegexSearch(1:string query),
	i32 GetNumKeys(),
	i64 GetShardSize(),
	
	ServerHeartBeat Ping(),
}