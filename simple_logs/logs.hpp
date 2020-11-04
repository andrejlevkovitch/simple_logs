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
enum class Severity { Info, Debug, Warning, Throw, Error, Failure };

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
  }

  standardLogFormat % fileName % lineNumber % functionName % timePoint %
      threadId % message;

  return standardLogFormat.str();
}

class BasicLogger {
public:
  virtual ~BasicLogger() = default;

  virtual void log(Severity         severity,
                   std::string_view fileName,
                   int              lineNumber,
                   std::string_view functionName,
                   std::chrono::time_point<std::chrono::system_clock> timePoint,
                   std::thread::id                                    threadId,
                   boost::format message) noexcept = 0;
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

    if (Severity::Failure == severity) {
      exit(EXIT_FAILURE);
    }
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
 * \note this is slowly operation, so if you not need logging time you should
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
#  define LOG_FORMAT(severity, message)                                        \
    LOGGER.log(severity,                                                       \
               __FILE__,                                                       \
               __LINE__,                                                       \
               __func__,                                                       \
               GET_LOG_TIME(),                                                 \
               GET_LOG_THREAD_ID(),                                            \
               message)
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
    LOG_FORMAT(logs::Severity::Failure, logs::messageHandler(__VA_ARGS__))
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
