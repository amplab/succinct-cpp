service SuccinctService {
	i32 connect_to_handlers(),
  	i32 disconnect_from_handlers(),

  	i32 connect_to_local_servers(),
  	i32 disconnect_from_local_servers(),

  	i32 start_servers(),
	i32 initialize(1:i32 mode),
	
	string get(1:i64 key),
    string get_local(1:i32 qserver_id, 2:i64 key),
    
    string get_all(1:i64 key)
	
	i32 get_num_hosts(),
	i32 get_num_shards(1:i32 host_id),
	i32 get_num_keys(1:i32 shard_id),
}

service QueryService {
    i32 init(1:i32 id),
	string get(1:i64 key),
	
	i32 get_num_keys(),
}

service MasterService {
  	string get_client(),
}
