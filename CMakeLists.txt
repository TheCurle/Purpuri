#project config

cmake_minimum_required(VERSION 3.10)

project(purpuri)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB src_files
    "source/*.cpp",
    "source/class/*.cpp"
)

if(WIN32)

else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -O0")
endif()

include_directories("headers")

add_executable(purpuri ${src_files})