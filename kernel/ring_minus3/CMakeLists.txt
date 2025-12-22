# KERNEL BYPASS FRAMEWORK - Ring -3 Firmware
# CMake build configuration (reference only)

cmake_minimum_required(VERSION 3.20)
project(KernelBypass_Firmware LANGUAGES C)

message(WARNING "
================================================================================
  FIRMWARE / UEFI BUILD NOTICE
================================================================================

  ⚠️ EXTREME DANGER ZONE ⚠️
  
  Firmware modification can:
  - Permanently brick your hardware
  - Void warranties
  - Have legal implications
  
  Requirements:
  1. EDK2 (TianoCore UEFI Development Kit)
  2. Intel ME SDK (for ME exploitation)
  3. SPI programmer for recovery
  4. Dedicated test hardware (NEVER use production!)
  
  Build with EDK2:
    source edksetup.sh
    build -a X64 -t GCC5 -p KernelBypassPkg/KernelBypassPkg.dsc
  
  Test hardware recommendations:
  - Cheap Intel NUC or similar
  - Board with dual BIOS or SPI recovery
  - External SPI programmer (CH341A, etc.)
  
================================================================================
")

# Source files (for reference)
set(UEFI_SOURCES
    uefi/bootkit.c
    uefi/bootkit.h
    uefi/runtime_driver.c
    uefi/hooks.c
    uefi/patches.c
)

set(SMM_SOURCES
    ../ring_minus2/smm/smm_handler.c
    ../ring_minus2/smm/smm_handler.h
)

set(ME_SOURCES
    me/me_exploit.c
    me/heci.c
)

message(STATUS "UEFI sources: ${UEFI_SOURCES}")
message(STATUS "SMM sources: ${SMM_SOURCES}")
message(STATUS "ME sources: ${ME_SOURCES}")

