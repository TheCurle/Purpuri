#project config

# May God have mercy on your soul.

# Windows: Download SDL2, put the libraries in lib/windows/
# MacOS: brew install sdl2


cmake_minimum_required(VERSION 3.14)

project(purpuri)

set(DEBUGGER False)

# ******************************************
# CMAKE defines

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# CMAKE dependencies
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_compile_options(
    # gcc and clang: debugging symbols
    "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-g>" 

    # gcc and clang: warnings become errors
    #"$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Werror>"
    # msvc: warnings are fatal
    "$<$<CXX_COMPILER_ID:MSVC>:/WX>"

    # gcc and clang: show all warnings
    "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall>"
    # gcc and clang: show extra warnings
    "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wextra>"
    # msvc: show all warnings
    "$<$<CXX_COMPILER_ID:MSVC>:/Wall>"
    # gcc and clang: enable pedantic mode when compiling
    "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-pedantic>"
    # msvc: enable permissive mode
    "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
    
    # clang: ambiguous-reversed-operator; workaround for a C++ standard bug.
    "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-error=ambiguous-reversed-operator>"
    # clang: bad-function-cast; workaround for an Imgui bug.
    "$<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-error=bad-function-cast>" 
    # gcc: cast-function-type; workaround for an Imgui bug.
    "$<$<CXX_COMPILER_ID:GNU>:-Wno-error=cast-function-type>"
    
    # clang and gcc: disable all optimizations.
    "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-O0>"

    # windows: mingw and msys; use ANSI compatible printf, workaround a gcc bug.
    "$<$<PLATFORM_ID:Windows>:-D__USE_MINGW_ANSI_STDIO>"
)

# ******************************************
# Source files

file(GLOB vm_src CONFIGURE_DEPENDS
    "source/vm/*.cpp"
    "source/vm/class/*.cpp"
    "source/vm/native/*.cpp"
    "source/vm/debug/*.cpp"
    "source/miniz.c"
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
target_link_libraries(purpuri PRIVATE Threads::Threads ZLIB::ZLIB)

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

    # windows: manually link against sdl because there's no standard location
    if(WIN32)
        add_library(sdl IMPORTED SHARED)
        set_target_properties(sdl PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/windows/SDL2.dll IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/lib/windows/libSDL2.dll.a)
        target_link_libraries(purpuri PRIVATE sdl)
    else()
    # macos and linux: just look for sdl where it is expected to be.
        find_package(SDL2 REQUIRED)
        target_link_libraries(purpuri PRIVATE ${SDL2_LIBRARIES})
    endif()
    

    find_package(OpenGL)

    # windows and linux: use -lopengl32
    if(WIN32 OR LINUX)
        target_link_libraries(purpuri PRIVATE opengl32)
    endif()

    # macos: use the special bunny command line argument that apple wants
    if(APPLE) 
        target_link_libraries(purpuri PRIVATE "-framework OpenGL")
    endif()
    
endif()
