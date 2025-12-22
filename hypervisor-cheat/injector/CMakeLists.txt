# HYPERVISOR CHEAT - Injector
# Simple LoadLibrary injector for testing

cmake_minimum_required(VERSION 3.20)
project(hypervisor_injector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(injector main.cpp)

target_compile_definitions(injector PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

if(MSVC)
    target_compile_options(injector PRIVATE
        $<$<CONFIG:Release>:/O2 /GL>
        /W3
    )
    target_link_options(injector PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

set_target_properties(injector PROPERTIES
    OUTPUT_NAME "hv_injector"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

