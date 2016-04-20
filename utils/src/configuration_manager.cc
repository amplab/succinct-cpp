#include "configuration_manager.h"

#include <cstdlib>

#include "constants.h"

ConfigurationManager::ConfigurationManager() {
  // System configuration parameters
  SetFromEnv("CONF_PATH", std::string(Defaults::kConfPath));
  SetFromEnv("LOG_PATH", std::string(Defaults::kLogPath));
  SetFromEnv("MASTER_HOSTNAME", std::string(Defaults::kCoordinatorHostname));
  SetFromEnv("HOSTS_LIST", Get("CONF_PATH") + "/" + Defaults::kHostsFile);

  // Coordinator configuration parameters
  SetFromEnv("COORDINATOR_LOG_LEVEL", std::to_string(Defaults::kLogLevel));
  SetFromEnv("COORDINATOR_LOG_FILE",
             Get("LOG_PATH") + "/" + Defaults::kCoordinatorLogFile);
  SetFromEnv("COORDINATOR_PORT", std::to_string(Defaults::kCooordinatorPort));

  // Server configuration parameters
  SetFromEnv("SERVER_LOG_LEVEL", std::to_string(Defaults::kLogLevel));
  SetFromEnv("SERVER_LOG_PATH_PREFIX",
             Get("LOG_PATH") + "/" + Defaults::kServerLogFilePrefix);
  SetFromEnv("SERVER_PORT", std::to_string(Defaults::kServerPort));
  SetFromEnv("LOAD_MODE", std::to_string(Defaults::kLoadMode));
  SetFromEnv("SHARDS_PER_SERVER", std::to_string(Defaults::kShardsPerServer));

  // Core configuration parameters
  SetFromEnv("SA_SAMPLING_RATE", std::to_string(Defaults::kSASamplingRate));
  SetFromEnv("ISA_SAMPLING_RATE", std::to_string(Defaults::kISASamplingRate));
  SetFromEnv("NPA_SAMPLING_RATE", std::to_string(Defaults::kNPASamplingRate));
  SetFromEnv("SA_SAMPLING_SCHEME", std::to_string(Defaults::kSASamplingScheme));
  SetFromEnv("ISA_SAMPLING_SCHEME",
             std::to_string(Defaults::kISASamplingScheme));
  SetFromEnv("NPA_SCHEME", std::to_string(Defaults::kNPASamplingScheme));
}

const std::string ConfigurationManager::ReadEnvStr(
    const char *env_var, const std::string& default_str) {
  if (const char* env_p = std::getenv(env_var)) {
    return std::string(env_p);
  }
  return default_str;
}

void ConfigurationManager::SetFromEnv(const std::string &key,
                                      const std::string& default_value) {
  conf[key] = ReadEnvStr(key.c_str(), default_value);
}

void ConfigurationManager::Set(const std::string &key,
                               const std::string &value) {
  conf[key] = value;
}

const std::string ConfigurationManager::Get(const std::string &key) {
  if (conf.find(key) != conf.end()) {
    return conf.at(key);
  }
  std::string error_msg = "Configuration parameter " + key + " does not exist.";
  throw ConfigNotFoundException(error_msg);
}

const bool ConfigurationManager::GetBool(const std::string &key) {
  return Get(key) == "true";
}

const int8_t ConfigurationManager::GetByte(const std::string &key) {
  return (int8_t) atoi(Get(key).c_str());
}

const int16_t ConfigurationManager::GetShort(const std::string &key) {
  return (int16_t) atoi(Get(key).c_str());
}

const int32_t ConfigurationManager::GetInt(const std::string &key) {
  return atoi(Get(key).c_str());
}

const int64_t ConfigurationManager::GetLong(const std::string &key) {
  return atol(Get(key).c_str());
}
