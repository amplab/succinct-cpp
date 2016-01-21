#include "configuration_manager.h"

#include <cstdlib>

#include "constants.h"

ConfigurationManager::ConfigurationManager() {
  // System configuration parameters
  SetFromEnv("CONF_PATH", Defaults::kConfPath);
  SetFromEnv("LOG_PATH", Defaults::kLogPath);
  SetFromEnv("HOSTS_LIST", Get("CONF_PATH") + "/" + Defaults::kHostsFile);

  // Master configuration parameters
  SetFromEnv("MASTER_LOG_LEVEL", std::to_string(Defaults::kLogLevel));
  SetFromEnv("MASTER_LOG_FILE", Get("LOG_PATH") + "/" + Defaults::kMasterLogFile);
  SetFromEnv("MASTER_PORT", std::to_string(Defaults::kMasterPort));

  // Handler configuration parameters
  SetFromEnv("HANDLER_LOG_LEVEL", std::to_string(Defaults::kLogLevel));
  SetFromEnv("HANDLER_LOG_PATH_PREFIX", Get("LOG_PATH") + "/" + Defaults::kHandlerLogFilePrefix);
  SetFromEnv("HANDLER_PORT", std::to_string(Defaults::kHandlerPort));

  // Server configuration parameters
  SetFromEnv("SERVER_LOG_LEVEL", std::to_string(Defaults::kLogLevel));
  SetFromEnv("SERVER_LOG_PATH_PREFIX", Get("LOG_PATH") + "/" + Defaults::kServerLogFilePrefix);
  SetFromEnv("SERVER_PORT", std::to_string(Defaults::kServerPort));
  SetFromEnv("LOAD_MODE", std::to_string(Defaults::kLoadMode));
  SetFromEnv("SHARDS_PER_SERVER", std::to_string(Defaults::kShardsPerServer));

  // Core configuration parameters
  SetFromEnv("SA_SAMPLING_RATE", std::to_string(Defaults::kSASamplingRate));
  SetFromEnv("ISA_SAMPLING_RATE", std::to_string(Defaults::kISASamplingRate));
  SetFromEnv("NPA_SAMPLING_RATE", std::to_string(Defaults::kNPASamplingRate));
  SetFromEnv("SA_SAMPLING_SCHEME", std::to_string(Defaults::kSASamplingScheme));
  SetFromEnv("ISA_SAMPLING_SCHEME", std::to_string(Defaults::kISASamplingScheme));
  SetFromEnv("NPA_SCHEME", std::to_string(Defaults::kNPASamplingScheme));
}

const std::string ConfigurationManager::ReadEnvStr(const char *env_var, const std::string& default_str) {
  if(const char* env_p = std::getenv(env_var)) {
    return std::string(env_p);
  }
  return default_str;
}

void ConfigurationManager::SetFromEnv(const std::string &key, const std::string& default_value) {
  conf[key] = ReadEnvStr(key.c_str(), default_value);
}

void ConfigurationManager::Set(const std::string &key, const std::string &value) {
  conf[key] = value;
}

const std::string ConfigurationManager::Get(const std::string &key) {
  return conf.at(key);
}

const bool ConfigurationManager::GetBool(const std::string &key) {
  return conf.at(key) == "true";
}

const int8_t ConfigurationManager::GetByte(const std::string &key) {
  return (int8_t) atoi(conf.at(key).c_str());
}

const int16_t ConfigurationManager::GetShort(const std::string &key) {
  return (int16_t) atoi(conf.at(key).c_str());
}

const int32_t ConfigurationManager::GetInt(const std::string &key) {
  return atoi(conf.at(key).c_str());
}

const int64_t ConfigurationManager::GetLong(const std::string &key) {
  return atol(conf.at(key).c_str());
}
