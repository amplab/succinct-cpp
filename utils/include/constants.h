#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#include "logger.h"

class Constants {
 public:
  static const uint64_t kShardKeysMax = 1ULL << 32;
};

class Defaults {
 public:
  static constexpr const char* kConfPath = "conf/";
  static constexpr const char* kHostsFile = "hosts";
  static constexpr const char* kLogPath = "log/";
  static const int32_t kLogLevel = Logger::INFO;

  // Master defaults
  static constexpr const char* kMasterLogFile = "master.log";
  static const uint16_t kMasterPort = 11000;

  // Handler defaults
  static constexpr const char* kHandlerLogFilePrefix = "handler";
  static const uint16_t kHandlerPort = 11001;


  // Server defaults
  static constexpr const char* kServerLogFilePrefix = "server";
  static const int32_t kShardsPerServer = 1;
  static const int32_t kLoadMode = 3;
  static const uint16_t kServerPort = 11002;

  // Core defaults
  static const int32_t kSASamplingRate = 32;
  static const int32_t kISASamplingRate = 32;
  static const int32_t kNPASamplingRate = 128;
  static const int32_t kSASamplingScheme = 0;
  static const int32_t kISASamplingScheme = 0;
  static const int32_t kNPASamplingScheme = 1;

};

#endif /* DEFAULTS_H_ */
