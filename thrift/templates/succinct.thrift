service AggregatorService {
    i32 ConnectToServers(),
    i32 DisconnectFromServers(),
    i32 Initialize(),

    string Get(1:i64 key),
    string Access(1:i64 key, 2:i32 offset, 3:i32 len),
    set<i64> Search(1:string query),
    set<i64> Regex(1:string query),
    i64 Count(1:string query),
    string FlatExtract(1:i64 offset, 2:i64 length),
    i64 FlatCount(1:string query),
    list<i64> FlatSearch(1:string query),

    i32 GetNumShards(),
    i32 GetNumKeys(),
    i64 GetTotSize(),
}

service QueryService {
    i32 Initialize(1:i32 id),
    
    string Get(1:i64 key),
    string Access(1:i64 key, 2:i32 offset, 3:i32 len),
    set<i64> Search(1:string query),
    set<i64> Regex(1:string query),
    i64 Count(1:string query),
    
    string FlatExtract(1:i64 offset, 2:i64 length),
    i64 FlatCount(1:string query),
    list<i64> FlatSearch(1:string query), 
    
    i32 GetNumKeys(),
    i64 GetShardSize(),
}
