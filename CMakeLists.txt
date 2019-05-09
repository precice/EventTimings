cmake_minimum_required (VERSION 3.9.6)
project(EventTimings VERSION 1.0.0 LANGUAGES CXX)

# CMake <= 3.13
if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()

find_package(MPI REQUIRED)

add_library(EventTimings src/dummy.cpp)
set_target_properties(EventTimings PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  PUBLIC_HEADER "include/EventTimings/Event.hpp;include/EventTimings/EventUtils.hpp"
  )
target_include_directories(EventTimings
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  )
target_sources(EventTimings
  PRIVATE
  src/Event.cpp
  src/EventUtils.cpp
  src/TableWriter.cpp
  )
target_link_libraries(EventTimings PUBLIC MPI::MPI_CXX)


#
# Tests
#

enable_testing()

# Note:
# We do not link against EventTimings here, but compile it in.
# This makes debugging easier.
add_executable(testevents
  src/testevents.cpp
  src/Event.cpp
  src/EventUtils.cpp
  src/TableWriter.cpp
  )
target_link_libraries(testevents PRIVATE MPI::MPI_CXX)
target_include_directories(testevents PRIVATE src include)
set_target_properties(testevents PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
add_test(NAME EventTimings.events COMMAND testevents)


add_executable(testtable 
  src/testtable.cpp
  src/TableWriter.cpp
  )
target_include_directories(testtable PRIVATE src include)
set_target_properties(testtable PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
add_test(NAME EventTimings.table COMMAND testtable)


#
# Installation
#

install(TARGETS EventTimings
  EXPORT EventTimingsTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include/EventTimings
  INCLUDES DESTINATION include/EventTimings
)

install(EXPORT EventTimingsTargets
  FILE EventTimingsTargets.cmake
  NAMESPACE EventTimings::
  DESTINATION lib/cmake/EventTimings
)

# Generate Version Config
include(CMakePackageConfigHelpers)
write_basic_package_version_file("EventTimingsConfigVersion.cmake"
  VERSION ${EventTimings_VERSION}
  COMPATIBILITY SameMajorVersion
  )

# Install the Config and the ConfigVersion files
install(FILES "cmake/EventTimingsConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/EventTimingsConfigVersion.cmake"
  DESTINATION lib/cmake/EventTimings
)

# Setup Config in the Binary Directory
export(EXPORT EventTimingsTargets
  NAMESPACE EventTimings::
  FILE "EventTimingsTargets.cmake")
file(COPY cmake/EventTimingsConfig.cmake DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# Add Alias for subprojects
add_library(EventTimings::EventTimings ALIAS EventTimings)

install(FILES extra/events2trace.py DESTINATION share/EventTimings)
