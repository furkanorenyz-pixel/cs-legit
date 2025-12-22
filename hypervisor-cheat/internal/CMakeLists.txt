# HYPERVISOR CHEAT - Internal DLL
# Injected version with VMT hooks + ImGui

cmake_minimum_required(VERSION 3.20)
project(hypervisor_internal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Options
# ============================================

option(USE_IMGUI "Build with ImGui support" OFF)
option(DEBUG_CONSOLE "Allocate debug console" OFF)

# ============================================
# Sources
# ============================================

set(SOURCES
    src/dllmain.cpp
    src/hooks.cpp
    src/esp.cpp
)

set(HEADERS
    include/hooks.hpp
    include/esp.hpp
    include/memory.hpp
    include/xorstr.hpp
    include/lazy_import.hpp
)

# ============================================
# ImGui (optional)
# ============================================

if(USE_IMGUI)
    # Fetch ImGui
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.1
    )
    FetchContent_MakeAvailable(imgui)
    
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    
    list(APPEND SOURCES ${IMGUI_SOURCES})
endif()

# ============================================
# DLL Target
# ============================================

add_library(hypervisor_internal SHARED
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(hypervisor_internal PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../common
)

if(USE_IMGUI)
    target_include_directories(hypervisor_internal PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_compile_definitions(hypervisor_internal PRIVATE HAS_IMGUI)
endif()

target_link_libraries(hypervisor_internal PRIVATE
    d3d11
    dxgi
    dwmapi
    psapi
)

target_compile_definitions(hypervisor_internal PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

if(DEBUG_CONSOLE)
    target_compile_definitions(hypervisor_internal PRIVATE _DEBUG)
endif()

# ============================================
# Compiler Options
# ============================================

if(MSVC)
    target_compile_options(hypervisor_internal PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL /GS->
        /W4
        /wd4100  # unreferenced parameter
        /wd4189  # local variable not referenced
        /wd4244  # conversion warning
        /wd4267  # size_t conversion
    )
    
    target_link_options(hypervisor_internal PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

# ============================================
# Output
# ============================================

set_target_properties(hypervisor_internal PROPERTIES
    OUTPUT_NAME "hv_internal"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================
# Install
# ============================================

install(TARGETS hypervisor_internal
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# ============================================
# Info
# ============================================

message(STATUS "")
message(STATUS "╔═══════════════════════════════════════════════════════════════╗")
message(STATUS "║  HYPERVISOR CHEAT - Internal DLL                              ║")
message(STATUS "╠═══════════════════════════════════════════════════════════════╣")
message(STATUS "║  ImGui:         ${USE_IMGUI}")
message(STATUS "║  Debug Console: ${DEBUG_CONSOLE}")
message(STATUS "╚═══════════════════════════════════════════════════════════════╝")
message(STATUS "")
