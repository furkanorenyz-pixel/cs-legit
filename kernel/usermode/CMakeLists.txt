# KERNEL BYPASS FRAMEWORK - Usermode Library
# Universal interface for all ring levels

cmake_minimum_required(VERSION 3.20)
project(KernelBypass_Usermode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library source files
set(LIB_SOURCES
    src/interface.cpp
)

set(LIB_HEADERS
    include/kernel_interface.hpp
    ../common/types.h
    ../common/ioctl_codes.h
)

# Build as static library
add_library(kernel_bypass STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(kernel_bypass PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# Windows-specific
if(WIN32)
    target_compile_definitions(kernel_bypass PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(kernel_bypass PRIVATE
        /W4
        /WX-
        /O2
    )
endif()

# Example usage target
add_executable(kb_example example/main.cpp)
target_link_libraries(kb_example PRIVATE kernel_bypass)

message(STATUS "
================================================================================
  KERNEL BYPASS - Usermode Library
================================================================================

  This library provides a unified interface to all ring levels:
  - Ring 3: ReadProcessMemory / Direct Syscall
  - Ring 0: Kernel Driver (driver.sys)
  - Ring -1: Hypervisor
  - Ring -2/-3: SMM / Firmware
  
  Usage:
    #include <kernel_interface.hpp>
    
    int main() {
        kb::KernelInterface ki;
        if (ki.Initialize()) {
            // Auto-detected best backend
            printf(\"Using: %s\\n\", ki.GetBackendName());
            
            // Read memory from CS2
            DWORD pid = ki.FindProcess(\"cs2.exe\");
            auto base = ki.GetModuleBase(pid, \"client.dll\");
            auto value = ki.Read<int>(pid, *base + 0x1234);
        }
        return 0;
    }
  
================================================================================
")

