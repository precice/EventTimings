cmake_minimum_required (VERSION 3.1)
project(EventTimings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()


find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(COMPILE_FLAGS  ${COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(LINK_FLAGS ${LINK_FLAGS} ${MPI_LINK_FLAGS})

add_library(EventTimings EventTimings.cpp)
# add_library(TableWriter TableWriter.cpp)


add_executable(testevents testevents.cpp EventTimings.cpp TableWriter.cpp)
target_link_libraries(testevents ${MPI_LIBRARIES})
# target_link_libraries(testevents EventTimings)

add_executable(testtable testtable.cpp TableWriter.cpp)
# target_link_libraries(testtable TableWriter)


