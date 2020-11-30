// logs.hpp
/**\file
 * Before using logs you need create frontend and backend for logger. You can
 * use logs::StandardFrontend and logs::TextStreamBackend as default. Just
 * create std::shared_ptr from this and call LOGGER_ADD_SINK with frontend and
 * backend as parameters
 *
 * You can set your own specific format for your message. For do it you
 * need define `DEFAULT_LOG_FORMAT` macro as c-string. The string can contains
 * 8 items defined by:
 *
 * - `SEVERITY`
 * - `FILE_NAME`
 * - `LINE_NUMBER`
 * - `FUNCTION_NAME`
 * - `TIME_POINT`
 * - `THREAD_ID`
 * - `MESSAGE`
 *
 * You can combain the defines as you want.
 *
 * Also you can create you own format with some other entities, but note, that
 * you need your own frontend for it \see BasicFrontend.
 *
 * You can redefine logging macroses as you want. It can be done before
 * `#include` directive, or after (in second case be shure that you use `#undef`
 * macro.
 *
 * \warning be careful with redefining `LOG_THROW` and `LOG_FAILURE` macroses,
 * because it can has unexpected behaviour if `LOG_THROW` doesn't throw
 * exception or if `LOG_FAILURE` doesn't exit from program
 *
 * \warning be carefull with using logging in destructors. Logger is a static
 * object, but there is no guarantie that it will not destroy before some other
 * static objects or etc. If you get error like
 * > pure virtual function called
 * then it can signalized about using logger object after its destruction
 */

#pragma once

#include <boost/format.hpp>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <string_view>
#include <thread>

#define TRACE_SEVERITY   "TRC"
#define DEBUG_SEVERITY   "DBG"
#define INFO_SEVERITY    "INF"
#define WARNING_SEVERITY "WRN"
#define THROW_SEVERITY   "THR"
#define ERROR_SEVERITY   "ERR"
#define FAILURE_SEVERITY "FLR"

// All args set in specific order for formatting
#define SEVERITY      "%1%"
#define FILE_NAME     "%2%"
#define LINE_NUMBER   "%3%"
#define FUNCTION_NAME "%4%"
#define TIME_POINT    "%5%"
#define THREAD_ID     "%6%"
#define MESSAGE       "%7%"

#define MESSAGE_PREFIX "|"

#define STANDARD_LOG_FORMAT                                                    \
  SEVERITY " " THREAD_ID " [" TIME_POINT "] " FILE_NAME ":" LINE_NUMBER        \
           " " FUNCTION_NAME " " MESSAGE_PREFIX " " MESSAGE

#define LIGHT_LOG_FORMAT                                                       \
  SEVERITY " " FILE_NAME ":" LINE_NUMBER " " MESSAGE_PREFIX " " MESSAGE

#ifndef DEFAULT_LOG_FORMAT
#  define DEFAULT_LOG_FORMAT LIGHT_LOG_FORMAT
#endif

namespace logs {
enum class Severity {
  /// use in predicates
  Placeholder,
  Trace,
  Debug,
  Info,
  Warning,
  Throw,
  Error,
  Failure
};

inline std::string toString(Severity sev) {
  switch (sev) {
  case Severity::Trace:
    return TRACE_SEVERITY;
  case Severity::Debug:
    return DEBUG_SEVERITY;
  case Severity::Info:
    return INFO_SEVERITY;
  case Severity::Warning:
    return WARNING_SEVERITY;
  case Severity::Error:
    return ERROR_SEVERITY;
  case Severity::Failure:
    return FAILURE_SEVERITY;
  case Severity::Throw:
    return THROW_SEVERITY;
  default:
    assert(false && "invalid severity");
  }

  return "";
}
} // namespace logs

namespace std {
/**\brief print time
 * \note this function is not thread safe
 */
inline std::ostream &
operator<<(std::ostream &stream,
           const std::chrono::time_point<std::chrono::system_clock>
               &timePoint) noexcept {
  std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
  stream << std::put_time(std::localtime(&t), "%c");
  return stream;
}

inline std::ostream &operator<<(std::ostream &stream, logs::Severity sev) {
  stream << logs::toString(sev);
  return stream;
}
} // namespace std

namespace logs {
class SeverityPredicat {
public:
  explicit SeverityPredicat(std::function<bool(Severity)> foo) noexcept
      : predicate_{std::move(foo)} {
  }

  SeverityPredicat(Severity sev) noexcept
      : predicate_{[sev](Severity input) {
        return static_cast<int>(sev) == static_cast<int>(input);
      }} {
  }

  bool operator()(Severity input) const {
    return predicate_(input);
  }

  operator bool() {
    return predicate_.operator bool();
  }

private:
  std::function<bool(Severity)> predicate_;
};

inline SeverityPredicat operator&&(const SeverityPredicat &lhs,
                                   const SeverityPredicat &rhs) {
  SeverityPredicat retval{[lhs, rhs](Severity input) {
    return lhs(input) && rhs(input);
  }};
  return retval;
}

inline SeverityPredicat operator||(const SeverityPredicat &lhs,
                                   const SeverityPredicat &rhs) {
  SeverityPredicat retval{[lhs, rhs](Severity input) {
    return lhs(input) || rhs(input);
  }};
  return retval;
}

[[noreturn]] inline bool invalidPredicate(Severity) {
  throw std::runtime_error{"invalid predicate calling"};
}

inline SeverityPredicat
makePredicate(Severity                      lhs,
              Severity                      rhs,
              std::function<bool(int, int)> op) noexcept {
  if (lhs == Severity::Placeholder) {
    return SeverityPredicat{[checkVal = rhs, op = op](Severity input) {
      return op(static_cast<int>(input), static_cast<int>(checkVal));
    }};
  } else if (rhs == Severity::Placeholder) {
    return SeverityPredicat{[checkVal = lhs, op = op](Severity input) {
      return op(static_cast<int>(checkVal), static_cast<int>(input));
    }};
  } else {
    if (op(static_cast<int>(lhs), static_cast<int>(rhs))) {
      return SeverityPredicat{invalidPredicate}; // convertible to true
    }

    return SeverityPredicat{nullptr}; // convertible to false
  }
}

inline SeverityPredicat operator!=(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::not_equal_to<int>());
}

inline SeverityPredicat operator==(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::equal_to<int>());
}

inline SeverityPredicat operator<(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::less<int>());
}

inline SeverityPredicat operator>(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::greater<int>());
}

inline SeverityPredicat operator<=(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::less_equal<int>());
}

inline SeverityPredicat operator>=(Severity lhs, Severity rhs) {
  return makePredicate(lhs, rhs, std::greater_equal<int>());
}

inline SeverityPredicat operator!(Severity val) {
  return makePredicate(Severity::Placeholder, val, std::not_equal_to<int>());
}

/**\return safety format object for user message
 */
inline boost::format getLogFormat(std::string_view format) noexcept {
  boost::format retval{std::string{format}};
  retval.exceptions(boost::io::all_error_bits ^ (boost::io::too_few_args_bit |
                                                 boost::io::too_many_args_bit));
  return retval;
}

/**\brief help function for combine all user arguments in one message
 */
template <typename... Args>
boost::format doFormat(boost::format format, Args... args) noexcept {
  return (format % ... % args);
}

/**\brief formatting user message
 */
template <typename... Args>
boost::format messageHandler(std::string_view messageFormat,
                             Args... args) noexcept {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format), args...);
}

class BasicFrontend {
public:
  BasicFrontend()
      : filter_{Severity::Placeholder >= Severity::Trace} {
  }
  virtual ~BasicFrontend() = default;

  virtual std::string makeRecord(Severity         severity,
                                 std::string_view fileName,
                                 int              lineNumber,
                                 std::string_view functionName,
                                 boost::format    message) const noexcept = 0;

  /**\throw exception if filter is invalid
   */
  void setFilter(SeverityPredicat filter) noexcept(false) {
    if (filter == false) {
      throw std::invalid_argument{"invalid severity filter"};
    }

    filter_ = std::move(filter);
  }

  SeverityPredicat getFilter() const noexcept {
    return filter_;
  }

private:
  SeverityPredicat filter_;
};

class StandardFrontend final : public BasicFrontend {
public:
  std::string makeRecord(Severity         severity,
                         std::string_view fileName,
                         int              lineNumber,
                         std::string_view functionName,
                         boost::format    message) const noexcept override {
    boost::format standardLogFormat{STANDARD_LOG_FORMAT};
    standardLogFormat.exceptions(boost::io::all_error_bits ^
                                 boost::io::too_many_args_bit);

    standardLogFormat % severity % fileName % lineNumber % functionName %
        std::chrono::system_clock::now() % std::this_thread::get_id() % message;

    return standardLogFormat.str();
  }
};

/**\brief like a StandardFrontend, but don't use time
 */
class LightFrontend final : public BasicFrontend {
public:
  std::string makeRecord(Severity         severity,
                         std::string_view fileName,
                         int              lineNumber,
                         std::string_view functionName,
                         boost::format    message) const noexcept override {
    boost::format standardLogFormat{LIGHT_LOG_FORMAT};
    standardLogFormat.exceptions(boost::io::all_error_bits ^
                                 boost::io::too_many_args_bit);

    standardLogFormat % severity % fileName % lineNumber % functionName % "" %
        "" % message;

    return standardLogFormat.str();
  }
};

class BasicBackend {
public:
  virtual ~BasicBackend() = default;

  /**\brief get record from frontend
   * \note that this function can call from several threads, so you need prevent
   * data race by using mutex
   */
  virtual void consume(std::string_view record) noexcept = 0;
};

class TextStreamBackend final : public BasicBackend {
public:
  explicit TextStreamBackend(std::ostream &stream)
      : stream_{stream} {
  }

  /**\note uses mutex
   */
  void consume(std::string_view record) noexcept override {
    std::lock_guard<std::mutex> lock{mutex_};
    stream_ << record << std::endl;
  }

private:
  std::ostream &stream_;
  std::mutex    mutex_;
};

struct Sink {
  std::shared_ptr<BasicFrontend> frontend;
  std::shared_ptr<BasicBackend>  backend;
};

class SimpleLogger {
public:
  void log(Severity         severity,
           std::string_view fileName,
           int              lineNumber,
           std::string_view functionName,
           boost::format    message) noexcept {
    // XXX in case of using several threads here can be a data race, because
    // sink creates in main thread, but this function can be called from other
    // thread. But it is ok, becuase we don't change sink, frontend or backend
    // in this function
    for (Sink &sink : sinks_) {
      if (sink.frontend->getFilter()(severity)) {
        std::string record = sink.frontend->makeRecord(severity,
                                                       fileName,
                                                       lineNumber,
                                                       functionName,
                                                       message);
        sink.backend->consume(record);
      }
    }
  }

  /**\throw exception if frontend or backend are invalid
   */
  void addSink(Sink sink) noexcept(false) {
    if (sink.frontend == nullptr) {
      throw std::invalid_argument{"invalid logger frontend"};
    }
    if (sink.backend == nullptr) {
      throw std::invalid_argument{"invalid logger backend"};
    }

    sinks_.emplace_back(std::move(sink));
  }

  static SimpleLogger &get() noexcept {
    static SimpleLogger logger;
    return logger;
  }

private:
  SimpleLogger() noexcept {
  }

  SimpleLogger(const SimpleLogger &) = delete;
  SimpleLogger(SimpleLogger &&)      = delete;

private:
  std::list<Sink> sinks_;
};
} // namespace logs

#define LOGGER logs::SimpleLogger::get()

/**\brief add new sink for logger
 * \warning use it before calling any log macroses! And do not add additional
 * sinks after calling some logging macroses. This can have unexpected behaviour
 */
#define LOGGER_ADD_SINK(frontend, backend)                                     \
  LOGGER.addSink(logs::Sink{frontend, backend})

#ifndef LOG_FORMAT
#  define LOG_FORMAT(severity, message)                                        \
    LOGGER.log(severity, __FILE__, __LINE__, __func__, message);
#endif

#ifndef LOG_TRACE
#  define LOG_TRACE(...)                                                       \
    LOG_FORMAT(logs::Severity::Trace, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_DEBUG
#  define LOG_DEBUG(...)                                                       \
    LOG_FORMAT(logs::Severity::Debug, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_INFO
#  define LOG_INFO(...)                                                        \
    LOG_FORMAT(logs::Severity::Info, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_WARNING
#  define LOG_WARNING(...)                                                     \
    LOG_FORMAT(logs::Severity::Warning, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_ERROR
#  define LOG_ERROR(...)                                                       \
    LOG_FORMAT(logs::Severity::Error, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_FAILURE
/**\brief print log and terminate program
 * \warning be careful with redefining! `LOG_FAILURE` must finish program,
 * otherwise it can has unexpected behaviour
 */
#  define LOG_FAILURE(...)                                                     \
    LOG_FORMAT(logs::Severity::Failure, logs::messageHandler(__VA_ARGS__));    \
    exit(EXIT_FAILURE)
#endif

#ifndef LOG_THROW
/**\brief print log and generate specified exception
 * \param ExceptionType type of generated exception
 * \warning be careful with redefining! `LOG_THROW` must throw needed exception,
 * otherwise it can has unexpected behaviour
 */
#  define LOG_THROW(ExceptionType, ...)                                        \
    {                                                                          \
      boost::format fmt = logs::messageHandler(__VA_ARGS__);                   \
      LOG_FORMAT(logs::Severity::Throw, fmt);                                  \
      throw ExceptionType{fmt.str()};                                          \
    }
#endif
