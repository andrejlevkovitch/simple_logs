# cmake

cmake_minimum_required(VERSION 3.12)

project(logs)

include(build.cmake)

find_package(Boost REQUIRED)

add_library(simple_logs INTERFACE)
target_include_directories(simple_logs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(simple_logs INTERFACE Boost::boost)
target_compile_features(simple_logs INTERFACE cxx_std_17)


if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  include(doxygen.cmake)

  add_executable(check_logs main.cpp)
  target_link_libraries(check_logs PRIVATE simple_logs)
endif()
