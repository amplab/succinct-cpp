include "heartbeat.thrift"

namespace cpp succinct

service Master {
	string GetHostname(),

  // I-Am-Alive messages from handler
  void Ping(1:heartbeat.HeartBeat hb),
}
