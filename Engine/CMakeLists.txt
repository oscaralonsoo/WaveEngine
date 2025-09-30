cmake_minimum_required(VERSION 3.10)

project(Engine)

find_package(fmt CONFIG REQUIRED)

add_executable(Engine Main.cpp)

target_link_libraries(Engine PRIVATE fmt::fmt)