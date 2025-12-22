# KERNEL BYPASS FRAMEWORK - Ring -2 SMM
# CMake build configuration (reference only)

cmake_minimum_required(VERSION 3.20)
project(KernelBypass_SMM LANGUAGES C)

message(WARNING "
================================================================================
  SMM (System Management Mode) BUILD NOTICE
================================================================================

  Ring -2 runs BELOW the OS and hypervisors!
  
  SMM code has:
  - Full access to all memory (including SMRAM)
  - Full access to all I/O ports
  - Invisible to OS and antivirus
  
  Requirements:
  1. EDK2 for UEFI/SMM development
  2. Understanding of SMRAM/TSEG
  3. Knowledge of SMI handlers
  4. SPI programmer for recovery
  
  Testing:
  - Use dedicated test hardware ONLY
  - Have SPI programmer ready for recovery
  - Test in VM first (limited SMM emulation)
  
================================================================================
")

set(SMM_SOURCES
    smm/smm_handler.c
    smm/smm_handler.h
    smm/smm_comm.c
)

message(STATUS "SMM sources: ${SMM_SOURCES}")

