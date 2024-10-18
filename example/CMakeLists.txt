cmake_minimum_required(VERSION 3.0)
project(executor)

set(CMAKE_CXX_STANDARD 17)


add_executable(
    checker
    checker.cpp
)

target_link_libraries(checker PRIVATE kexec)
