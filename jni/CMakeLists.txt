cmake_minimum_required(VERSION 3.22)
project(timewheel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8")
set(CMAKE_VERBOSE_MAKEFILE ON)

message("$ENV{JAVA_HOME}")
include_directories($ENV{JAVA_HOME}/include)
include_directories(include)

if (WIN32)
    set(JAVA_MD_PATH win32)
else ()
    set(JAVA_MD_PATH linux)
endif ()
include_directories($ENV{JAVA_HOME}/include/${JAVA_MD_PATH})

add_subdirectory(src)
add_subdirectory(test)
