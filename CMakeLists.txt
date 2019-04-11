cmake_minimum_required (VERSION 3.9)
project(EventTimings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()

find_package(MPI REQUIRED)


include_directories("src")
add_subdirectory("src")

add_library(EventTimings ${sourcesEventTimings})
target_compile_features(EventTimings PUBLIC cxx_std_11)
target_link_libraries(EventTimings PUBLIC MPI::MPI_CXX)

set_target_properties(EventTimings PROPERTIES PUBLIC_HEADER
  "src/Event.hpp src/EventUtils.hpp")

# We do not link against EventTimings here, but compile it in.
# This makes debugging easier.
add_executable(testevents ${sourcesTestevents})
target_link_libraries(testevents PRIVATE MPI::MPI_CXX)

add_executable(testtable ${sourcesTesttable})
# target_link_libraries(testtable TableWriter)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/events2trace.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
