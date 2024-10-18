cmake_minimum_required(VERSION 3.15)

project(
    kexec
    VERSION 1.0
    DESCRIPTION "Execution Library"
    LANGUAGES CXX
)

add_subdirectory(contrib)

include_directories(include)

add_library(
    kexec
    
    src/kexec.cpp
    src/types.cpp
)

target_link_libraries(
    kexec PRIVATE fmt::fmt
)

set_target_properties(
    kexec PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(
    kexec PUBLIC include
)

add_library(Kexec::Kexec ALIAS kexec)

