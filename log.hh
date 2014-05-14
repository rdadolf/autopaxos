#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip> // setprecision
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "json.hh"

#include <string.h> // strerror
#include <errno.h> // errno types

#define INFO() LogInstance("INFO",__FILE__,__LINE__)
#define WARN() LogInstance("WARN",__FILE__,__LINE__)
#define ERROR() LogInstance("ERROR",__FILE__,__LINE__)
#define DATA() LogInstance("DATA",__FILE__,__LINE__)

class LogState {
private:
  std::ofstream logfile_;

  LogState() {
    logfile_.open("log.txt", std::ios_base::out | std::ios_base::app);
  }
public:
  static LogState& get() {
    static LogState ls;
    return ls;
  }

  template<typename T>LogState &operator<< (T rhs) {
    logfile_ << rhs;
    logfile_.flush();
    return *this;
  }
};

class LogInstance {
private:
  std::stringstream buffer_;
  const char *mode_, *file_;
  int line_;
public:
  LogInstance(const char *mode, const char *file, const int line) : mode_(mode), file_(file), line_(line), buffer_("") {
  }

  template<typename T> LogInstance& operator<< (T rhs) {
    buffer_ << rhs;
    return *this;
  }
  ~LogInstance() {
    struct timeval tv;

    gettimeofday(&tv, 0);
    LogState::get() << std::fixed << std::setprecision(6)
                    << double(tv.tv_sec*1000000+tv.tv_usec)/1000000.
                    << std::setprecision(0)
                    << " [" << getpid() << ":" << file_ << ":" << line_ << "] "
                    << mode_ << ": " << buffer_.str() << "\n";
  }
};

#endif // __LOG_H__
