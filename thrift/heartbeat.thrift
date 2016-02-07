struct HeartBeat {
	1: i64 timestamp,
  2: i32 sender_id,
}

struct HeartBeatResponse {
  1: list<double> health,
}
