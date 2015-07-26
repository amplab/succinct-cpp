service AdaptiveSuccinctService {
	i32 connect_to_handlers(1:i32 client_id),			// Do not expose
    i32 disconnect_from_handlers(1:i32 client_id),		// Do not expose
    
    i32 connect_to_local_servers(1:i32 client_id),		// Do not expose
    i32 disconnect_from_local_servers(1:i32 client_id),	// Do not expose

    i32 start_servers(),								// Do not expose
    i32 initialize(1:i32 mode),							// Do not expose								

    i32 get_request(1:i32 client_id, 2:i64 key),
    i64 get_request_local(1:i32 client_id, 2:i32 local_shard_id, 3:i64 key),	// Do not expose
    string get_response(1:i32 client_id, 2:i32 replica_id),
    string get_response_local(1:i32 client_id, 2:i32 local_shard_id),			// Do not expose
    
    i64 remove_layer(1:i32 shard_id, 2:i32 layer_id),						
    i64 reconstruct_layer(1:i32 shard_id, 2:i32 layer_id),
   
    i32 get_num_shards(),
    i32 get_num_keys(1:i32 shard_id),
    i64 get_queue_length(1:i32 shard_id),
    i64 storage_size(1:i32 shard_id),
}

service AdaptiveQueryService {
    i32 init(1:i32 id),
    string get(1:i64 key),
    list<string> batch_get(1:list<i64> keys),
    string access(1:i64 key, 2:i32 offset, 3:i32 len),
    list<string> batch_access(1:list<i64> keys, 2:i32 offset, 3:i32 len),
    set<i64> search(1:string query),
    i64 count(1:string query),
    i32 get_num_keys(),
    i64 remove_layer(1:i32 layer_id),
    i64 reconstruct_layer(1:i32 layer_id),
    i64 storage_size(),
    i64 num_sampled_values(),
}

service AdaptiveMasterService {
    string get_client(),
    i32 get_client_id(),
}
