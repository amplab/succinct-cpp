service SuccinctService {
	string extract(1:i64 offset, 2:i64 len),
	i64 count(1:string query),
	list<i64> search(1:string query),
	list<i64> wildcard_search(1:string pattern, 2:i64 max_sep),
}

service QueryService {
    i32 init(),
	string extract(1:i64 offset, 2:i64 len),
	i64 count(1:string query),
	list<i64> search(1:string query),
	list<i64> wildcard_search(1:string pattern, 2:i64 max_sep),
}
