#project config

cmake_minimum_required(VERSION 3.14)

project(purpuri)

set(DEBUGGER False)

# ******************************************
# CMAKE defines

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# -Wno-error=cast-function-type; workaround for an Imgui bug.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -Wno-error=cast-function-type -O0 -D__USE_MINGW_ANSI_STDIO -pthread")

# ******************************************
# Source files

file(GLOB vm_src CONFIGURE_DEPENDS
    "source/vm/*.cpp"
    "source/vm/class/*.cpp"
    "source/vm/native/*.cpp"
    "source/vm/debug/*.cpp"
)

file(GLOB lib_src CONFIGURE_DEPENDS
    "source/lib/*.cpp"
)

include_directories("inc")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ******************************************
# If the debugger is enabled, compile necessary code inside the repos.

if (DEFINED debugger)
    add_compile_definitions(VISUAL_DEBUGGER)
endif()


# ******************************************
# Setup the Purpuri executable.
# It's compiled with ENABLE_EXPORTS so that we can read VM functions from native code.


add_executable(purpuri)
target_sources(purpuri PRIVATE ${vm_src})
set_target_properties(purpuri PROPERTIES ENABLE_EXPORTS ON)
target_compile_definitions(purpuri PRIVATE VM_BUILDING)

# Linux needs to compile against -ldl
if (UNIX)
    target_link_libraries(purpuri PRIVATE dl)
endif()


# ******************************************
# Setup the native library.

add_library(native MODULE ${lib_src})
target_link_libraries(native PRIVATE purpuri)


# ******************************************
# Setup the visual debugger, if enabled
# ******************************************

if (DEFINED debugger)
    file(GLOB imgui_src CONFIGURE_DEPENDS
        "source/vm/debug/imgui/*.cpp"
    )

    target_sources(purpuri PRIVATE ${imgui_src})
    add_library(sdl IMPORTED SHARED)
    set_target_properties(sdl PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/SDL2.dll IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/lib/libSDL2.dll.a)
    target_link_libraries(purpuri PRIVATE sdl)

    find_package(OpenGL)
    target_link_libraries(purpuri PRIVATE opengl32)
endif()
