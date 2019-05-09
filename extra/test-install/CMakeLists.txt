cmake_minimum_required (VERSION 3.9.6)
project(Dummy LANGUAGES CXX)

find_package(EventTimings REQUIRED)

add_executable(Dummy dummy.cpp)
target_link_libraries(Dummy EventTimings::EventTimings)
