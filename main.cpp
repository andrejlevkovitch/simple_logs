// main.cpp

#include "simple_logs/logs.hpp"
#include <cstdlib>

int main(int argc, char *argv[]) {
  auto coutBackend   = std::make_shared<logs::TextStreamBackend>(std::cout);
  auto cerrBackend   = std::make_shared<logs::TextStreamBackend>(std::cerr);
  auto debugFrontend = std::make_shared<logs::LightFrontend>();
  auto errorFrontend = std::make_shared<logs::LightFrontend>();

  debugFrontend->setFilter(logs::Severity::Placeholder ==
                               logs::Severity::Debug ||
                           logs::Severity::Placeholder == logs::Severity::Info);
  errorFrontend->setFilter(logs::Severity::Placeholder >=
                           logs::Severity::Warning);

  LOGGER_ADD_SINK(debugFrontend, coutBackend);
  LOGGER_ADD_SINK(errorFrontend, cerrBackend);

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
