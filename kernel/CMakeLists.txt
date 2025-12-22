# ============================================
# KERNEL BYPASS FRAMEWORK
# Universal Multi-Ring Bypass Library
# ============================================
#
# Ring 0:   Kernel Driver (MmCopyVirtualMemory, DKOM, HWID Spoof)
# Ring -1:  Hypervisor (VT-x EPT Hooking, CPUID Spoof)
# Ring -2:  SMM (System Management Mode)
# Ring -3:  Firmware (UEFI Bootkit, Intel ME)
#
# ============================================

cmake_minimum_required(VERSION 3.20)
project(KernelBypassFramework VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Options
# ============================================

option(BUILD_USERMODE "Build usermode library" ON)
option(BUILD_RING0 "Build kernel driver (requires WDK)" OFF)
option(BUILD_HYPERVISOR "Build hypervisor (requires WDK + VT-x)" OFF)
option(BUILD_FIRMWARE "Build firmware components (requires EDK2)" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)

# ============================================
# Output directories
# ============================================

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================
# Print Configuration
# ============================================

message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════════════════╗")
message(STATUS "║         KERNEL BYPASS FRAMEWORK v${PROJECT_VERSION}                    ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║                                                              ║")
message(STATUS "║  Components:                                                 ║")
message(STATUS "║    • Ring 0  - Kernel Driver     [${BUILD_RING0}]                       ║")
message(STATUS "║    • Ring -1 - Hypervisor        [${BUILD_HYPERVISOR}]                       ║")
message(STATUS "║    • Ring -2 - SMM Handler       [${BUILD_FIRMWARE}]                       ║")
message(STATUS "║    • Ring -3 - UEFI/ME           [${BUILD_FIRMWARE}]                       ║")
message(STATUS "║    • Usermode Library            [${BUILD_USERMODE}]                        ║")
message(STATUS "║                                                              ║")
message(STATUS "╚══════════════════════════════════════════════════════════════╝")
message(STATUS "")

# ============================================
# Usermode Library (always builds on Windows)
# ============================================

if(BUILD_USERMODE AND WIN32)
    add_subdirectory(usermode)
endif()

# ============================================
# Kernel Components (require WDK)
# ============================================

if(BUILD_RING0)
    message(STATUS "")
    message(STATUS "  ⚠️  Ring 0 requires Windows Driver Kit (WDK)")
    message(STATUS "      Use Visual Studio with WDK to build driver.sys")
    message(STATUS "")
    # add_subdirectory(ring0)  # Would need WDK integration
endif()

if(BUILD_HYPERVISOR)
    message(STATUS "")
    message(STATUS "  ⚠️  Hypervisor requires:")
    message(STATUS "      - Intel CPU with VT-x OR AMD CPU with AMD-V")
    message(STATUS "      - VT-x/AMD-V enabled in BIOS")
    message(STATUS "      - Windows Driver Kit (WDK)")
    message(STATUS "")
    # add_subdirectory(ring_minus1)
endif()

if(BUILD_FIRMWARE)
    message(STATUS "")
    message(STATUS "  ⚠️  Firmware requires:")
    message(STATUS "      - EDK2 (TianoCore)")
    message(STATUS "      - SPI programmer for recovery")
    message(STATUS "      - DEDICATED TEST HARDWARE!")
    message(STATUS "")
    # add_subdirectory(ring_minus2)
    # add_subdirectory(ring_minus3)
endif()

# ============================================
# Summary
# ============================================

message(STATUS "")
message(STATUS "Build configuration complete.")
message(STATUS "")
message(STATUS "To build usermode library:")
message(STATUS "  cmake -B build -G Ninja")
message(STATUS "  cmake --build build")
message(STATUS "")
message(STATUS "For kernel/hypervisor/firmware:")
message(STATUS "  See README.md for detailed instructions")
message(STATUS "")

