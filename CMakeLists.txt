cmake_minimum_required (VERSION 2.8)
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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(COMPILE_FLAGS  ${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(LINK_FLAGS ${LINK_FLAGS} ${MPI_LINK_FLAGS})

add_library(EventTimings EventTimings.cpp TableWriter.cpp)

add_executable(testevents testevents.cpp EventTimings.cpp TableWriter.cpp)
target_link_libraries(testevents ${MPI_LIBRARIES})
# target_link_libraries(testevents EventTimings)

add_executable(testtable testtable.cpp TableWriter.cpp)
# target_link_libraries(testtable TableWriter)


