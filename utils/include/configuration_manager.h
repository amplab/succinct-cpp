#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <map>
#include <string>

class ConfigNotFoundException : std::exception {
 public:
  ConfigNotFoundException(const std::string& msg)
      : msg_(msg) {
  }

  virtual const char *what() const throw () {
    return msg_.c_str();
  }

 private:
  const std::string msg_;
};

class ConfigurationManager {
 public:
  typedef std::map<std::string, std::string> Properties;

  ConfigurationManager();

  /**
   * Set configuration property.
   */
  void Set(const std::string& key, const std::string& value);

  /**
   * Get configuration property.
   */
  const std::string Get(const std::string& key);
  const bool GetBool(const std::string& key);
  const int8_t GetByte(const std::string& key);
  const int16_t GetShort(const std::string& key);
  const int32_t GetInt(const std::string& key);
  const int64_t GetLong(const std::string& key);

  const std::string ReadEnvStr(const char *env_var,
                               const std::string& default_str = "");

 private:
  void SetFromEnv(const std::string& key, const std::string& default_value);

  Properties conf;
};

#endif // CONFIGURATION_MANAGER_H
