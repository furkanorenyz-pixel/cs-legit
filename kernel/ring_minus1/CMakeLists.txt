# KERNEL BYPASS FRAMEWORK - Ring -1 Hypervisor
# CMake build configuration (reference only)

cmake_minimum_required(VERSION 3.20)
project(KernelBypass_Hypervisor LANGUAGES C ASM_MASM)

message(WARNING "
================================================================================
  HYPERVISOR BUILD NOTICE
================================================================================

  Hypervisor requires:
  
  1. Intel CPU with VT-x OR AMD CPU with AMD-V
  2. VT-x/AMD-V enabled in BIOS
  3. Hyper-V DISABLED on Windows
  4. Visual Studio 2022 + WDK
  
  Testing Requirements:
  - VMware Workstation with nested VT-x
  - OR dedicated test hardware
  
  NEVER test hypervisor code on production system!
  
================================================================================
")

# Source files (for reference)
set(HYPERVISOR_SOURCES
    hypervisor/vmx.c
    hypervisor/vmx.h
    hypervisor/ia32.h
    hypervisor/ept.c
    hypervisor/ept.h
    hypervisor/vmexit.c
    hypervisor/vmx_asm.asm
)

message(STATUS "Hypervisor sources: ${HYPERVISOR_SOURCES}")

