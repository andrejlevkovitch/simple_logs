# cmake

cmake_minimum_required(VERSION 3.12)

project(logs)

include(build.cmake)
include(doxygen.cmake)

find_package(Boost REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost)
