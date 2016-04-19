include "heartbeat.thrift"

namespace cpp succinct

service Coordinator {
	string GetHostname(),

  // I-Am-Alive messages from server
  void Ping(1:heartbeat.HeartBeat hb),
}
