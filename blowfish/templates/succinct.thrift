service SuccinctService {
    i32 ConnectToHandlers(),
    i32 DisconnectFromHandlers(),

    i32 ConnectToLocalServers(),
    i32 DisconnectFromLocalServers(),

    i32 StartLocalServers(),
    i32 Initialize(1:i32 mode),

    string Get(1:i32 shard_id, 2:i64 key),
    string GetLocal(1:i32 qserver_id, 2:i64 key),

    set<i64> Search(1:i32 shard_id, 2:string query),
    set<i64> SearchLocal(1:i32 qserver_id, 2:string query),
    
    i32 GetNumHosts(),
    i32 GetNumShards(1:i32 host_id),
    i32 GetNumKeys(1:i32 shard_id),
    i64 GetTotSize(),
}

service QueryService {
    i32 Initialize(1:i32 id, 2:i32 sampling_rate),
    string Get(1:i64 key),
    set<i64> Search(1:string query),
    i32 GetNumKeys(),
    i64 GetShardSize(),
}

service MasterService {
    string GetClient(),
}
