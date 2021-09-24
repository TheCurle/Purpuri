#project config

cmake_minimum_required(VERSION 3.10)

project(purpuri)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB src_files
    "source/vm/*.cpp",
    "source/vm/class/*.cpp",
    "source/vm/native/*.cpp"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(WIN32)

else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -O0")
endif()

include_directories("inc")

add_executable(purpuri ${src_files})