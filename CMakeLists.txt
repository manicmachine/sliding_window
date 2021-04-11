cmake_minimum_required(VERSION 3.11.4)
project(sliding_window)

set(CMAKE_CXX_STANDARD 11)
set(BOOST_ROOT "/mnt/csather/boost_1_75_0")
include_directories(${BOOST_ROOT})
add_executable(sliding_window main.cpp packet.h PacketBuilder.cpp PacketBuilder.h Packet.h ApplicationState.h ApplicationState.h PacketInfo.h InputHelper.cpp InputHelper.h Connection.h ConnectionController.cpp ConnectionController.h ConnectionSettings.h)