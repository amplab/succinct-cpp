#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>

class Logger {
 public:
  enum Level {
    ALL = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
    OFF = 6
  };

  static const size_t MAX_PREFIX_SIZE = 40;

  /**
   * Constructor to initialize the logger.
   *
   * @param level The logging level for the logger.
   * @param out The output file descriptor for the logger.
   */
  Logger(Level level = Level::INFO, FILE* out = stderr);

  /**
   * Set the logging level.
   *
   * @param level The logging level for the logger.
   */
  void SetLevel(Level level);

  /**
   * Get the logging level.
   *
   * @return The logging level for the logger.
   */
  Level GetLevel();

  /**
   * Set the output descriptor.
   *
   * @param descriptor The output descriptor for the logger.
   */
  void SetDescriptor(FILE *descriptor);

  /**
   * Get the output descriptor.
   *
   * @return The output descriptor for the logger.
   */
  FILE* GetDescriptor();

  /**
   * Logs message as DEBUG.
   *
   * @param formatted_msg The formatted message.
   */
  void Debug(const char* formatted_msg, ...);

  /**
   * Logs message as INFO.
   *
   * @param formatted_msg The formatted message.
   */

  void Info(const char* formatted_msg, ...);

  /**
   * Logs message as WARN.
   *
   * @param formatted_msg The formatted message.
   */
  void Warn(const char* formatted_msg, ...);

  /**
   * Logs message as ERROR.
   *
   * @param formatted_msg The formatted message.
   */
  void Error(const char* formatted_msg, ...);

  /**
   * Logs message as FATAL.
   *
   * @param formatted_msg The formatted message.
   */
  void Fatal(const char* formatted_msg, ...);

 private:
  const std::string LogMsgPrefix(Level level);

  FILE* out_;
  Level level_;

};

#endif /* LOGGER_H_ */
