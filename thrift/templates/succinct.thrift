service SuccinctService {
    i32 ConnectToHandlers(),
    i32 DisconnectFromHandlers(),

    i32 ConnectToLocalServers(),
    i32 DisconnectFromLocalServers(),

    i32 StartLocalServers(),
    i32 Initialize(1:i32 mode),

    string Get(1:i64 key),
    string GetLocal(1:i32 qserver_id, 2:i64 key),

    string Access(1:i64 key, 2:i32 offset, 3:i32 len),
    string AccessLocal(1:i32 qserver_id, 2:i64 key, 3:i32 offset, 4:i32 len),

    set<i64> Search(1:string query),
    set<i64> SearchLocal(1:string query),
    
    set<i64> RegexSearch(1:string query),
    set<i64> RegexSearchLocal(1:string query),
    
    list<i64> RegexCount(1:string query),
    list<i64> RegexCountLocal(1:string query),

    i64 Count(1:string query),
    i64 CountLocal(1:string query),
    
    string FlatExtract(1:i64 offset, 2:i64 length),
    string FlatExtractLocal(1:i64 offset, 2:i64 length),
    
    i64 FlatCount(1:string query),
    i64 FlatCountLocal(1:string query),
    
    list<i64> FlatSearch(1:string query),
    list<i64> FlatSearchLocal(1:string query),

    i32 GetNumHosts(),
    i32 GetNumShards(1:i32 host_id),
    i32 GetNumKeys(1:i32 shard_id),
    i64 GetTotSize(),
}

service QueryService {
    i32 Initialize(1:i32 id),
    string Get(1:i64 key),
    string Access(1:i64 key, 2:i32 offset, 3:i32 len),
    set<i64> Search(1:string query),
    set<i64> RegexSearch(1:string query),
    list<i64> RegexCount(1:string query),
    i64 Count(1:string query),
    string FlatExtract(1:i64 offset, 2:i64 length),
    i64 FlatCount(1:string query),
    list<i64> FlatSearch(1:string query), 
    i32 GetNumKeys(),
    i64 GetShardSize(),
}

service MasterService {
    string GetClient(),
}
