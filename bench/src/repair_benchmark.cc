#include <iostream>
#include <fstream>
#include <unistd.h>

#include "MasterService.h"
#include "ports.h"
#include "benchmark.h"

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [-i]\n", exec);
}

int main(int argc, char **argv) {
  if (argc > 5) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  int32_t mode = 0;         // Defaults to replication
  int32_t host_id = 0;      // Defaults to host 0
  bool indefinite = false;
  while ((c = getopt(argc, argv, "m:h:i")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'h':
        host_id = atoi(optarg);
        break;
      case 'i':
        indefinite = true;
        break;
      default:
        fprintf(stderr, "Invalid option %c\n", c);
        exit(0);
    }
  }

  try {
    fprintf(stderr, "Connecting to master...\n");
    boost::shared_ptr<TSocket> socket(
        new TSocket("localhost", SUCCINCT_MASTER_PORT));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    MasterServiceClient *master = new MasterServiceClient(protocol);
    transport->open();
    fprintf(stderr, "Connected!\n");

    do {
      fprintf(stderr, "Starting repair...\n");
      Benchmark::TimeStamp start = Benchmark::GetTimestamp();
      master->RepairHost(host_id, mode);
      Benchmark::TimeStamp end = Benchmark::GetTimestamp();
      fprintf(stderr, "Complete! Time taken = %llu seconds\n",
              (end - start) / (1000 * 1000));

    } while(indefinite);

  } catch (std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
  }

  return 0;
  return 0;
}
