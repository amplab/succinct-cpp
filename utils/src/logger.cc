#include "logger.h"

Logger::Logger(Level level, FILE* out) {
  level_ = level;
  out_ = out;
}

void Logger::SetLevel(Level level) {
  level_ = level;
}

Logger::Level Logger::GetLevel() {
  return level_;
}

void Logger::SetDescriptor(FILE *descriptor) {
  out_ = descriptor;
}

FILE* Logger::GetDescriptor() {
  return out_;
}

void Logger::Debug(const char* formatted_msg, ...) {
  if (level_ > Level::DEBUG) {
    return;
  }
  va_list format_args;
  va_start(format_args, formatted_msg);
  std::string fmt_str = LogMsgPrefix(Level::DEBUG) + std::string(formatted_msg) + "\n";
  const char* fmt = fmt_str.c_str();
  vfprintf(out_, fmt, format_args);
  fflush(out_);
  va_end(format_args);
}

void Logger::Info(const char* formatted_msg, ...) {
  if (level_ > Level::INFO) {
    return;
  }
  va_list format_args;
  va_start(format_args, formatted_msg);
  std::string fmt_str = LogMsgPrefix(Level::INFO) + std::string(formatted_msg) + "\n";
  const char* fmt = fmt_str.c_str();
  vfprintf(out_, fmt, format_args);
  fflush(out_);
  va_end(format_args);
}

void Logger::Warn(const char* formatted_msg, ...) {
  if (level_ > Level::WARN) {
    return;
  }
  va_list format_args;
  va_start(format_args, formatted_msg);
  std::string fmt_str = LogMsgPrefix(Level::WARN) + std::string(formatted_msg) + "\n";
  const char* fmt = fmt_str.c_str();
  vfprintf(out_, fmt, format_args);
  fflush(out_);
  va_end(format_args);
}

void Logger::Error(const char* formatted_msg, ...) {
  if (level_ > Level::ERROR) {
    return;
  }
  va_list format_args;
  va_start(format_args, formatted_msg);
  std::string fmt_str = LogMsgPrefix(Level::ERROR) + std::string(formatted_msg) + "\n";
  const char* fmt = fmt_str.c_str();
  vfprintf(out_, fmt, format_args);
  fflush(out_);
  va_end(format_args);
}

void Logger::Fatal(const char* formatted_msg, ...) {
  if (level_ > Level::FATAL) {
    return;
  }
  va_list format_args;
  std::string fmt_str = LogMsgPrefix(Level::FATAL) + std::string(formatted_msg) + "\n";
  const char* fmt = fmt_str.c_str();
  vfprintf(out_, fmt, format_args);
  fflush(out_);
  va_end(format_args);
}

const std::string Logger::LogMsgPrefix(Level level) {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[MAX_PREFIX_SIZE];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  switch (level) {
    case Level::DEBUG: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S DEBUG ", timeinfo);
      break;
    }
    case Level::INFO: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S INFO ", timeinfo);
      break;
    }
    case Level::WARN: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S WARN ", timeinfo);
      break;
    }
    case Level::ERROR: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S ERROR ", timeinfo);
      break;
    }
    case Level::FATAL: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S FATAL ", timeinfo);
      break;
    }
    default: {
      strftime(buffer, MAX_PREFIX_SIZE, "%Y-%m-%d %H:%M:%S ??? ", timeinfo);
    }
  }

  return std::string(buffer);
}
