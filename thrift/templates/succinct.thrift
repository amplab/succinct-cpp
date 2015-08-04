service SuccinctService {
    i32 connect_to_handlers(),
    i32 disconnect_from_handlers(),

    i32 connect_to_local_servers(),
    i32 disconnect_from_local_servers(),

    i32 start_servers(),
    i32 initialize(1:i32 mode),

    string get(1:i64 key),
    string get_local(1:i32 qserver_id, 2:i64 key),

    string access(1:i64 key, 2:i32 offset, 3:i32 len),
    string access_local(1:i32 qserver_id, 2:i64 key, 3:i32 offset, 4:i32 len),

    set<i64> search(1:string query),
    set<i64> search_local(1:string query),
    
    set<i64> regex_search(1:string query),
    set<i64> regex_search_local(1:string query),
    
    list<i64> regex_count(1:string query),
    list<i64> regex_count_local(1:string query),

    i64 count(1:string query),
    i64 count_local(1:string query),
    
    string flat_extract(1:i64 offset, 2:i64 length),
    string flat_extract_local(1:i64 offset, 2:i64 length),
    
    i64 flat_count(1:string query),
    i64 flat_count_local(1:string query),
    
    list<i64> flat_search(1:string query),
    list<i64> flat_search_local(1:string query),

    i32 get_num_hosts(),
    i32 get_num_shards(1:i32 host_id),
    i32 get_num_keys(1:i32 shard_id),
    i64 get_tot_size(),
}

service QueryService {
    i32 init(1:i32 id),
    string get(1:i64 key),
    string access(1:i64 key, 2:i32 offset, 3:i32 len),
    set<i64> search(1:string query),
    set<i64> regex_search(1:string query),
    list<i64> regex_count(1:string query),
    i64 count(1:string query),
    string flat_extract(1:i64 offset, 2:i64 length),
    i64 flat_count(1:string query),
    list<i64> flat_search(1:string query), 
    i32 get_num_keys(),
    i64 get_shard_size(),
}

service MasterService {
    string get_client(),
}
