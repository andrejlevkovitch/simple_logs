// logs.hpp
/**\file
 * Also you can set your own specific format for your message. For do it you
 * need define `STANDARD_LOG_FORMAT` macro as c-string. The string can contains
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
 * You can redefine logging macroses as you want. It can be done before
 * `#include` directive, or after (in second case be shure that you use `#undef`
 * macro. Also you can redefine `GET_LOG_TIME` for avoid slow system calls
 *
 * For specify you own logger you need:
 * 1. inherit your logger from logs::BasicLogger @see logs::BasicLogger
 * 2. redefine LOGGER with your logger @see LOGGER
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
#include <string_view>
#include <thread>

#define INFO_SEVERITY    "INF"
#define DEBUG_SEVERITY   "DBG"
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

#ifndef STANDARD_LOG_FORMAT
/// default logging format
#  define STANDARD_LOG_FORMAT                                                  \
    "[" SEVERITY "] " FILE_NAME ":" LINE_NUMBER " " MESSAGE
#endif

namespace std {
/**\brief print time
 */
inline std::ostream &
operator<<(std::ostream &stream,
           const std::chrono::time_point<std::chrono::system_clock>
               &timePoint) noexcept {
  std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
  stream << std::put_time(std::localtime(&t), "%c");
  return stream;
}
} // namespace std

namespace logs {
enum class Severity {
  /// use in predicates
  Placeholder,
  Info,
  Debug,
  Warning,
  Throw,
  Error,
  Failure
};

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

inline boost::format doFormat(boost::format format) noexcept {
  return format;
}

/**\brief help function for combine all user arguments in one message
 */
template <typename T, typename... Args>
boost::format doFormat(boost::format format, T arg, Args... args) noexcept {
  format % arg;
  return doFormat(std::move(format), args...);
}

inline boost::format messageHandler(std::string_view messageFormat) noexcept {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format));
}

/**\brief formatting user message
 */
template <typename... Args>
boost::format messageHandler(std::string_view messageFormat,
                             Args... args) noexcept {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format), args...);
}

/**\brief combine message and metadata to one rectord for logging by
 * STANDARD_LOG_FORMAT
 * \see STANDARD_LOG_FORMAT
 */
inline std::string
getRecord(Severity                                           severity,
          std::string_view                                   fileName,
          int                                                lineNumber,
          std::string_view                                   functionName,
          std::chrono::time_point<std::chrono::system_clock> timePoint,
          std::thread::id                                    threadId,
          boost::format                                      message) noexcept {
  boost::format standardLogFormat{STANDARD_LOG_FORMAT};
  standardLogFormat.exceptions(boost::io::all_error_bits ^
                               boost::io::too_many_args_bit);

  switch (severity) {
  case Severity::Info:
    standardLogFormat % INFO_SEVERITY;
    break;
  case Severity::Debug:
    standardLogFormat % DEBUG_SEVERITY;
    break;
  case Severity::Warning:
    standardLogFormat % WARNING_SEVERITY;
    break;
  case Severity::Error:
    standardLogFormat % ERROR_SEVERITY;
    break;
  case Severity::Failure:
    standardLogFormat % FAILURE_SEVERITY;
    break;
  case Severity::Throw:
    standardLogFormat % THROW_SEVERITY;
    break;
  default:
    assert(false && "invalid severity");
    break;
  }

  standardLogFormat % fileName % lineNumber % functionName % timePoint %
      threadId % message;

  return standardLogFormat.str();
}

class BasicLogger {
public:
  BasicLogger() noexcept
      : filter_{Severity::Placeholder >= Severity::Info} {
  }
  virtual ~BasicLogger() = default;

  virtual void log(Severity         severity,
                   std::string_view fileName,
                   int              lineNumber,
                   std::string_view functionName,
                   std::chrono::time_point<std::chrono::system_clock> timePoint,
                   std::thread::id                                    threadId,
                   boost::format message) noexcept = 0;

  SeverityPredicat getFilter() const noexcept {
    return filter_;
  }

  void setFilter(SeverityPredicat filter) noexcept {
    if (filter) {
      filter_ = filter;
    } else {
      filter_ = Severity::Placeholder >= Severity::Info;
    }
  }

private:
  SeverityPredicat filter_;
};

/**\brief print log messages to terminal
 */
class TerminalLogger final : public BasicLogger {
public:
  void log(Severity                                           severity,
           std::string_view                                   fileName,
           int                                                lineNumber,
           std::string_view                                   functionName,
           std::chrono::time_point<std::chrono::system_clock> timePoint,
           std::thread::id                                    threadId,
           boost::format message) noexcept override {
    std::cerr << getRecord(severity,
                           fileName,
                           lineNumber,
                           functionName,
                           timePoint,
                           threadId,
                           std::move(message))
              << std::endl;
  }

  static TerminalLogger &get() noexcept {
    static TerminalLogger logger;
    return logger;
  }

private:
  TerminalLogger() noexcept = default;

  TerminalLogger(const TerminalLogger &) = delete;
  TerminalLogger(TerminalLogger &&)      = delete;
};
} // namespace logs

#ifndef GET_LOG_TIME
/**\brief get current time by system call
 * \warning this is slowly operation, so if you not need logging time you should
 * redefine the macros
 */
#  define GET_LOG_TIME() std::chrono::system_clock::now()
#endif

#ifndef GET_LOG_THREAD_ID
/**\brief get current thread id
 */
#  define GET_LOG_THREAD_ID() std::this_thread::get_id()
#endif

#ifndef LOGGER
#  define LOGGER logs::TerminalLogger::get()
#endif

#ifndef LOG_FORMAT
/**\note that LOG_FORMAT use logger filter for filtering logs
 */
#  define LOG_FORMAT(severity, message)                                        \
    if (LOGGER.getFilter()(severity)) {                                        \
      LOGGER.log(severity,                                                     \
                 __FILE__,                                                     \
                 __LINE__,                                                     \
                 __func__,                                                     \
                 GET_LOG_TIME(),                                               \
                 GET_LOG_THREAD_ID(),                                          \
                 message);                                                     \
    }
#endif

#ifndef LOG_INFO
#  define LOG_INFO(...)                                                        \
    LOG_FORMAT(logs::Severity::Info, logs::messageHandler(__VA_ARGS__))
#endif

#ifndef LOG_DEBUG
#  define LOG_DEBUG(...)                                                       \
    LOG_FORMAT(logs::Severity::Debug, logs::messageHandler(__VA_ARGS__))
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
