#project config

cmake_minimum_required(VERSION 3.14)
include(GenerateExportHeader)

project(purpuri)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB vm_src CONFIGURE_DEPENDS
    "source/vm/*.cpp",
    "source/vm/class/*.cpp",
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

add_library(native SHARED ${lib_src})
generate_export_header(native)

target_link_libraries(native PRIVATE purpuri)

if(WIN32)
    target_compile_options(purpuri PUBLIC ${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -O0 -Wl,--out-implib,libpurpuri.dll.a -Wl,--export-all-symbols -lwinpthread)
    target_compile_options(native PUBLIC -fpic -g -Werror -Wall -Wextra -O0 -shared)
else()
    target_compile_options(purpuri PUBLIC ${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -O0 -Wl,--out-implib,libpurpuri.dll.a -Wl,--export-all-symbols)
    target_compile_options(native PUBLIC -fpic -g -Werror -Wall -Wextra -O0 -shared)
endif()