cmake_minimum_required(VERSION 2.6)
project(AvogadroRpc)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Network Widgets REQUIRED)

set(AVOGADRO_RPC_SRCS
    main.cpp)

if(UNIX OR MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(UNIX OR MINGW)

add_executable(avogadroRpc ${AVOGADRO_RPC_SRCS})
target_link_libraries(avogadroRpc Qt5::Network Qt5::Widgets)
