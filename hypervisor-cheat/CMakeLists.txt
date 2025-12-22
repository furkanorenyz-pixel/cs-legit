# HYPERVISOR CHEAT - CMake Build System
# Ring -3 Bootkit → Ring -1 Hypervisor → Ring 3 Overlay

cmake_minimum_required(VERSION 3.20)
project(HypervisorCheat VERSION 1.0.0 LANGUAGES C CXX ASM_MASM)

# ============================================
# Global Settings
# ============================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================
# Common Headers
# ============================================

set(COMMON_INCLUDE ${CMAKE_SOURCE_DIR}/common)

# ============================================
# Usermode Overlay (Ring 3)
# ============================================

if(WIN32)
    add_executable(hypervisor_overlay
        usermode/src/main.cpp
        usermode/src/hypervisor.cpp
        usermode/src/esp.cpp
    )

    target_include_directories(hypervisor_overlay PRIVATE
        ${CMAKE_SOURCE_DIR}/usermode/include
        ${COMMON_INCLUDE}
    )

    target_link_libraries(hypervisor_overlay PRIVATE
        d3d11
        dxgi
        dwmapi
    )

    target_compile_definitions(hypervisor_overlay PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )

    # Optimizations for release
    if(MSVC)
        target_compile_options(hypervisor_overlay PRIVATE
            $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL>
            /W4
        )
        
        target_link_options(hypervisor_overlay PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    endif()
endif()

# ============================================
# Hypervisor (Ring -1) - Build separately
# ============================================

# The hypervisor requires special build environment
# See hypervisor/README.md for build instructions

# ============================================
# Bootkit (Ring -3) - Build separately with EDK2
# ============================================

# The bootkit requires EDK2 UEFI development kit
# See bootkit/README.md for build instructions

# ============================================
# Info
# ============================================

message(STATUS "")
message(STATUS "╔═══════════════════════════════════════════════════════════════╗")
message(STATUS "║  HYPERVISOR CHEAT Build Configuration                        ║")
message(STATUS "╠═══════════════════════════════════════════════════════════════╣")
message(STATUS "║  Components:                                                  ║")
message(STATUS "║    - Usermode Overlay: CMake (this project)                   ║")
message(STATUS "║    - Hypervisor: Custom build (see hypervisor/)               ║")
message(STATUS "║    - Bootkit: EDK2 (see bootkit/)                             ║")
message(STATUS "╚═══════════════════════════════════════════════════════════════╝")
message(STATUS "")

