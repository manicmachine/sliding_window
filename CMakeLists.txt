cmake_minimum_required(VERSION 3.17)
project(sliding_window)

set(CMAKE_CXX_STANDARD 11)

add_executable(sliding_window main.cpp packet.h PacketBuilder.cpp PacketBuilder.h RecController.cpp RecController.h SendController.cpp SendController.h Packet.h ApplicationState.h ApplicationState.h PacketInfo.h)