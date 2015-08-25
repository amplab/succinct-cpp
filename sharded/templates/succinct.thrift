service AggregatorService {
    i32 ConnectToServers(),
    i32 DisconnectFromServers(),
    i32 Initialize(),

    set<i64> Regex(1:string query),
    string Extract(1:i64 offset, 2:i64 length),
    i64 Count(1:string query),
    list<i64> Search(1:string query),

    i32 GetNumShards(),
    i64 GetTotSize(),
}

service QueryService {
    i32 Initialize(1:i32 id),
    
    set<i64> Regex(1:string query),
    string Extract(1:i64 offset, 2:i64 length),
    i64 Count(1:string query),
    list<i64> Search(1:string query), 
    
    i64 GetShardSize(),
}
