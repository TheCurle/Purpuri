#project config

cmake_minimum_required(VERSION 3.14)

project(purpuri)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -O0")

file(GLOB vm_src CONFIGURE_DEPENDS
    "source/vm/*.cpp"
    "source/vm/class/*.cpp"
    "source/vm/native/*.cpp"
)

file(GLOB lib_src CONFIGURE_DEPENDS
    "source/lib/*.cpp"
)

include_directories("inc")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(purpuri ${vm_src})
set_target_properties(purpuri PROPERTIES ENABLE_EXPORTS ON)
target_compile_definitions(purpuri PRIVATE VM_BUILDING)

add_library(native MODULE ${lib_src})

target_link_libraries(native PRIVATE purpuri)