#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "regex/regex.h"
#include "succinct_file.h"

// Debug
void display(RegEx *re) {
  switch (re->GetType()) {
    case RegExType::BLANK: {
      fprintf(stderr, "<blank>");
      break;
    }
    case RegExType::PRIMITIVE: {
      RegExPrimitive *p = ((RegExPrimitive *) re);
      std::string p_type;
      switch (p->GetPrimitiveType()) {
        case RegExPrimitiveType::MGRAM: {
          p_type = "mgram";
          break;
        }
        case RegExPrimitiveType::DOT: {
          p_type = "dot";
          break;
        }
        case RegExPrimitiveType::RANGE: {
          p_type = "range";
          break;
        }
        default:
          p_type = "unknown";
      }
      fprintf(stderr, "\"%s\":%s", p->GetPrimitive().c_str(), p_type.c_str());
      break;
    }
    case RegExType::REPEAT: {
      fprintf(stderr, "repeat(");
      display(((RegExRepeat *) re)->GetInternal());
      fprintf(stderr, ")");
      break;
    }
    case RegExType::CONCAT: {
      fprintf(stderr, "concat(");
      display(((RegExConcat *) re)->getLeft());
      fprintf(stderr, ",");
      display(((RegExConcat *) re)->getRight());
      fprintf(stderr, ")");
      break;
    }
    case RegExType::UNION: {
      fprintf(stderr, "union(");
      display(((RegExUnion *) re)->GetFirst());
      fprintf(stderr, ",");
      display(((RegExUnion *) re)->GetSecond());
      fprintf(stderr, ")");
      break;
    }
  }
}

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [-o] [file]\n", exec);
}

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
  struct timeval now;
  gettimeofday(&now, NULL);

  return (now.tv_usec + (time_t) now.tv_sec * 1000000) / 1000;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 5) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0;
  bool opt = false;
  while ((c = getopt(argc, argv, "m:o")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'o':
        opt = true;
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

  fprintf(stderr, "opt = %d\n", opt);

  std::string filename = std::string(argv[optind]);
  SuccinctFile *s_file = NULL;
  if (mode == 0) {
    std::cout << "Constructing Succinct data structures...\n";
    s_file = new SuccinctFile(filename);

    std::cout << "Serializing Succinct data structures...\n";
    std::ofstream s_out(filename + ".succinct");
    s_file->Serialize();
    s_out.close();
  } else {
    std::cout << "De-serializing Succinct data structures...\n";
    s_file = new SuccinctFile(filename, SuccinctMode::LOAD_IN_MEMORY);
  }

  std::cout << "Done.\n";

  while (true) {
    char exp[100];
    std::cout << "sure> ";
    std::cin.getline(exp, sizeof(exp));
    std::string query = std::string(exp);

    timestamp_t start = get_timestamp();
    SRegEx re(query, s_file, opt);

    fprintf(stderr, "Regex: [%s]\nExplanation: [", exp);
    re.Explain();
    fprintf(stderr, "]\n");

    re.Execute();

    timestamp_t tot_time = get_timestamp() - start;

    re.ShowResults(10);
    std::cout << "Query took " << tot_time << " ms\n";
  }

  return 0;
}
