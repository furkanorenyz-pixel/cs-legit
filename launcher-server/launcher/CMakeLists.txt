cmake_minimum_required(VERSION 3.16)
project(CheatLauncher VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    src/main.cpp
    src/api.cpp
    src/injector.cpp
)

# Executable
add_executable(launcher ${SOURCES})

# Windows-specific
if(WIN32)
    target_link_libraries(launcher PRIVATE
        winhttp
        ws2_32
        crypt32
    )
    
    # Static runtime for portability
    set_property(TARGET launcher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Include directories
target_include_directories(launcher PRIVATE include)

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(launcher PRIVATE /O2 /GL)
        target_link_options(launcher PRIVATE /LTCG)
    else()
        target_compile_options(launcher PRIVATE -O3)
    endif()
endif()

