cmake_minimum_required(VERSION 2.8.9)
project(chat-server)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++11)

add_executable(chat-server main.cpp)

find_library(ZMQ zmq)
target_link_libraries(chat-server ${ZMQ})