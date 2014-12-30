service SuccinctService {
	i32 connect_to_clients(),
  	i32 disconnect_from_clients(),

  	i32 connect_to_local_servers(),
  	i32 disconnect_from_local_servers(),

  	i32 start_servers(1:i32 num_servers, 2:i32 part_scheme),
	i32 initialize(1:i32 mode),
	
	string get(1:i64 key),
}

service QueryService {
    i32 init(1:i32 id),
	string get(1:i64 key),
}

service MasterService {
  	string get_client(),
}