#include "configuration_manager.h"

#include <cstdlib>

ConfigurationManager::ConfigurationManager() {
  SetFromEnv("HOSTS_LIST", "conf/hosts");
  SetFromEnv("MASTER_PORT", "11000");
  SetFromEnv("HANDLER_PORT", "11001");
  SetFromEnv("SERVER_PORT", "11002");
  SetFromEnv("NUM_SHARDS", "1");
  SetFromEnv("SA_SAMPLING_RATE", "32");
  SetFromEnv("ISA_SAMPLING_RATE", "32");
  SetFromEnv("NPA_SAMPLING_RATE", "128");
  SetFromEnv("SA_SAMPLING_SCHEME", "0");
  SetFromEnv("ISA_SAMPLING_SCHEME", "0");
  SetFromEnv("NPA_SCHEME", "1");
  SetFromEnv("LOAD_MODE", "0");
}

const std::string ConfigurationManager::ReadEnvStr(const char *env_var, const std::string& default_str) {
  if(const char* env_p = std::getenv(env_var)) {
    return std::string(env_p);
  }
  return default_str;
}

void ConfigurationManager::SetFromEnv(const std::string &key, const std::string& default_value) {
  conf[key] = ReadEnvStr(key.c_str());
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