// main.cpp
// for see all logs you need compile `Debug` version or define `NDEBUG`

// XXX uncomment for get colorized output in terminal
// #define COLORIZED

// XXX uncomment for get specific log output
// #define STANDARD_LOG_FORMAT \
//  TIME_POINT " [" THREAD_ID "] " TERMINAL_COLOR SEVERITY TERMINAL_NO_COLOR \
//             ":" FILE_NAME ":" LINE_NUMBER ":" FUNCTION_NAME \
//             ": " TERMINAL_COLOR MESSAGE TERMINAL_NO_COLOR

#include "simple_logs/logs.hpp"
#include <cstdlib>

int main(int argc, char *argv[]) {
  LOG_INFO("argc: %1%", argc);
  LOG_DEBUG("print only first argument: %1%; second argument never print",
            argc,
            argv);
  LOG_WARNING("some warning without arguments %1%");
  LOG_ERROR("some error", "never print");

  try {
    LOG_THROW(std::runtime_error,
              "some throw exception with argument: %1%",
              argc);
  } catch (std::runtime_error &) {
  }

  LOG_FAILURE("failure here even when logging switch off");

  std::cerr << "!!!never reachable!!!" << std::endl;

  return EXIT_SUCCESS;
}
