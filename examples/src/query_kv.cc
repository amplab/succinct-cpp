#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>
#include <sstream>

#include "succinct_shard.h"

/**
 * Prints usage.
 */
void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

void print_valid_cmds() {
  std::cerr
      << "Command must be one of: search [query], count [query], get [key]\n";
}

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
  struct timeval now{};
  gettimeofday(&now, nullptr);

  return (now.tv_usec + (time_t) now.tv_sec * 1000000);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 5) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0;
  while ((c = getopt(argc, argv, "m:")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Invalid option %c\n", c);
        exit(0);
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);

  SuccinctShard *s_file = nullptr;
  if (mode == 0) {
    // If mode is set to 0, compress the input file.
    // Use default parameters.
    std::cout << "Constructing Succinct data structures...\n";
    s_file = new SuccinctShard(0, filename);

    std::cout << "Serializing Succinct data structures...\n";
    s_file->Serialize(filename + ".succinct");
  } else {
    // If mode is set to 1, read the serialized data structures from disk.
    // The serialized data structures must exist at <filename>.succinct.
    std::cout << "De-serializing Succinct data structures...\n";
    s_file = new SuccinctShard(0, filename, SuccinctMode::LOAD_IN_MEMORY);
  }

  std::cout << "Done. Starting Succinct Shell...\n";

  print_valid_cmds();
  while (true) {
    char cmd_line[500];
    std::cout << "succinct> ";
    std::cin.getline(cmd_line, sizeof(cmd_line));
    std::istringstream iss(cmd_line);
    std::string cmd, arg;
    int64_t key;
    if (!(iss >> cmd)) {
      std::cerr << "Could not parse command: " << cmd_line << "\n";
      continue;
    }

    if (cmd == "search") {
      if (!(iss >> arg)) {
        std::cerr << "Could not parse argument: " << cmd_line << "\n";
        continue;
      }
      std::set<int64_t> results;
      timestamp_t start = get_timestamp();
      s_file->Search(results, arg);
      timestamp_t tot_time = get_timestamp() - start;
      std::cout << "Found " << results.size() << " records in " << tot_time
                << "us; Matching keys:\n";
      for (auto res : results) {
        std::cout << res << ", ";
      }
      std::cout << std::endl;
    } else if (cmd == "count") {
      if (!(iss >> arg)) {
        std::cerr << "Could not parse argument: " << cmd_line << "\n";
        continue;
      }
      timestamp_t start = get_timestamp();
      int64_t count = s_file->Count(arg);
      timestamp_t tot_time = get_timestamp() - start;
      std::cout << "Number of matching records = " << count << "; Time taken: "
                << tot_time << "us\n";
    } else if (cmd == "get") {
      if (!(iss >> key)) {
        std::cerr << "Could not parse argument: " << cmd_line << "\n";
        continue;
      }
      timestamp_t start = get_timestamp();
      std::string result;
      s_file->Get(result, key);
      timestamp_t tot_time = get_timestamp() - start;
      std::cout << "Value = " << result << "; Time taken: "
                << tot_time << "us\n";
    } else if (cmd == "exit") {
      break;
    } else {
      std::cerr << "Unsupported command: " << cmd << std::endl;
      print_valid_cmds();
    }
  }

  return 0;
}
