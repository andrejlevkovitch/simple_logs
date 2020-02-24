// logs.hpp
/**\file
 * If you want colorized color in console then just define `COLORIZED`.
 *
 * Also you can set your own specific format for your message. For do it you
 * need define `STANDARD_LOG_FORMAT` macro as c-string. The string can contains
 * 8 items defined by:
 *
 * - `SEVERITY`
 * - `CONSOLE_COLOR`
 * - `FILE_NAME`
 * - `LINE_NUMBER`
 * - `FUNCTION_NAME`
 * - `TIME_POINT`
 * - `THREAD_ID`
 * - `MESSAGE`
 *
 * You can combain the defines as you want.
 *
 * Also you must know that after every `CONSOLE_COLOR` must be
 * `CONSOLE_NO_COLOR`, otherwise you can get unexpected colorized output.
 * `CONSOLE_COLOR` accept only if `COLORIZED` defined.
 *
 * \warning Logging macroses produce output only if macro `NDEBUG` not defined,
 * BUT! `LOG_THROW` and `LOG_FAILURE` will work even if `NDEBUG` defined!
 */

#pragma once

#include <boost/format.hpp>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>

namespace std {
/**\brief print time
 */
inline std::ostream &operator<<(
    std::ostream &                                            stream,
    const std::chrono::time_point<std::chrono::system_clock> &timePoint) {
  std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
  stream << std::put_time(std::localtime(&t), "%c");
  return stream;
}
} // namespace std

namespace logs {
#define INFO_SEVERITY    "INF"
#define DEBUG_SEVERITY   "DBG"
#define WARNING_SEVERITY "WRN"
#define ERROR_SEVERITY   "ERR"
#define FAILURE_SEVERITY "FLR"

// for bash console
#define CONSOLE_NO_COLOR "\033[0m"
#define CONSOLE_BLUE     "\033[1;34m"
#define CONSOLE_GREEN    "\033[1;32m"
#define CONSOLE_YELLOW   "\033[1;33m"
#define CONSOLE_RED      "\033[1;31m"

#ifdef COLORIZED
#  define COLOR_INFO    CONSOLE_BLUE
#  define COLOR_DEBUG   CONSOLE_GREEN
#  define COLOR_WARNING CONSOLE_YELLOW
#  define COLOR_ERROR   CONSOLE_RED
#  define COLOR_FAILURE CONSOLE_RED
#  define COLOR_THROW   CONSOLE_YELLOW
#else
#  define COLOR_INFO    ""
#  define COLOR_DEBUG   ""
#  define COLOR_WARNING ""
#  define COLOR_ERROR   ""
#  define COLOR_FAILURE ""
#  define COLOR_THROW   ""
#endif

// All args set in specific order for formatting
#define SEVERITY      "%1%"
#define CONSOLE_COLOR "%2%"
#define FILE_NAME     "%3%"
#define LINE_NUMBER   "%4%"
#define FUNCTION_NAME "%5%"
#define TIME_POINT    "%6%"
#define THREAD_ID     "%7%"
#define MESSAGE       "%8%"

#ifndef STANDARD_LOG_FORMAT
/// default logging format
#  define STANDARD_LOG_FORMAT                                                  \
    CONSOLE_COLOR SEVERITY CONSOLE_NO_COLOR                                    \
        ":" FILE_NAME ":" LINE_NUMBER                                          \
        " " CONSOLE_COLOR MESSAGE CONSOLE_NO_COLOR
#endif

enum class Severity { Info, Debug, Warning, Error, Failure, Throw };

/**\return safety format object for user message
 */
inline boost::format getLogFormat(std::string_view format) {
  boost::format retval{std::string{format}};
  retval.exceptions(boost::io::all_error_bits ^ (boost::io::too_few_args_bit |
                                                 boost::io::too_many_args_bit));
  return retval;
}

inline boost::format doFormat(boost::format format) {
  return format;
}

/**\brief help function for combine all user arguments in one message
 */
template <typename T, typename... Args>
boost::format doFormat(boost::format format, T arg, Args... args) {
  format % arg;
  return doFormat(std::move(format), args...);
}

template <typename T>
boost::format messageHandler(T messageFormat) {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format));
}

/**\brief formatting user message
 */
template <typename T, typename... Args>
boost::format messageHandler(T messageFormat, Args... args) {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format), args...);
}

/**\brief print log messages to console
 */
inline void log(Severity                                           severity,
                std::string_view                                   fileName,
                int                                                lineNumber,
                std::string_view                                   functionName,
                std::chrono::time_point<std::chrono::system_clock> timePoint,
                std::thread::id                                    threadId,
                boost::format                                      message) {
  boost::format standardLogFormat{STANDARD_LOG_FORMAT};
  standardLogFormat.exceptions(boost::io::all_error_bits ^
                               boost::io::too_many_args_bit);

  std::ostream *stream;

  switch (severity) {
  case Severity::Info:
    stream = &std::cout;
    standardLogFormat % INFO_SEVERITY % COLOR_INFO;
    break;
  case Severity::Debug:
    stream = &std::cerr;
    standardLogFormat % DEBUG_SEVERITY % COLOR_DEBUG;
    break;
  case Severity::Warning:
    stream = &std::cerr;
    standardLogFormat % WARNING_SEVERITY % COLOR_WARNING;
    break;
  case Severity::Error:
    stream = &std::cerr;
    standardLogFormat % ERROR_SEVERITY % COLOR_ERROR;
    break;
  case Severity::Failure:
    stream = &std::cerr;
    standardLogFormat % FAILURE_SEVERITY % COLOR_FAILURE;
    break;
  case Severity::Throw:
    stream = &std::cerr;
    standardLogFormat % FAILURE_SEVERITY % COLOR_THROW;
  }

  standardLogFormat % fileName % lineNumber % functionName % timePoint %
      threadId % message;

  *stream << standardLogFormat << std::endl;

  if (Severity::Failure == severity) {
    exit(EXIT_FAILURE);
  }
}
} // namespace logs

#define LOG_FORMAT(severity, message)                                          \
  log(severity,                                                                \
      __FILE__,                                                                \
      __LINE__,                                                                \
      __func__,                                                                \
      std::chrono::system_clock::now(),                                        \
      std::this_thread::get_id(),                                              \
      message)

#ifdef NDEBUG

#  define LOG_INFO(...)
#  define LOG_DEBUG(...)
#  define LOG_WARNING(...)
#  define LOG_ERROR(...)

#else

#  define LOG_INFO(...)                                                        \
    LOG_FORMAT(logs::Severity::Info, logs::messageHandler(__VA_ARGS__))

#  define LOG_DEBUG(...)                                                       \
    LOG_FORMAT(logs::Severity::Debug, logs::messageHandler(__VA_ARGS__))

#  define LOG_WARNING(...)                                                     \
    LOG_FORMAT(logs::Severity::Warning, logs::messageHandler(__VA_ARGS__))

#  define LOG_ERROR(...)                                                       \
    LOG_FORMAT(logs::Severity::Error, logs::messageHandler(__VA_ARGS__))

#endif

/**\brief print log and terminate program
 * \warning work even if `NDEBUG` defined
 */
#define LOG_FAILURE(...)                                                       \
  LOG_FORMAT(logs::Severity::Failure, logs::messageHandler(__VA_ARGS__))

/**\brief print log and generate specified exception
 * \param ExceptionType type of generated exception
 * \warning work even if `NDEBUG` defined
 */
#define LOG_THROW(ExceptionType, ...)                                          \
  {                                                                            \
    boost::format fmt = logs::messageHandler(__VA_ARGS__);                     \
    LOG_FORMAT(logs::Severity::Throw, fmt);                                    \
    throw ExceptionType{fmt.str()};                                            \
  }
