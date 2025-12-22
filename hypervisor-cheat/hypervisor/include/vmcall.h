/**
 * HYPERVISOR CHEAT - VMCALL Handler
 * Handles communication between usermode and hypervisor
 */

#pragma once

#include "../../common/types.h"
#include "../../common/vmcall_codes.h"
#include "vmx.h"
#include "ept.h"
#include "safety.h"

// ============================================
// VMCALL Handler Context
// ============================================

typedef struct _VMCALL_CONTEXT {
    // Input (from guest)
    u64 Number;       // VMCALL number (from RAX)
    u64 Magic;        // Magic number (high 32 bits of RAX)
    u64 Param1;       // First parameter (RBX)
    u64 Param2;       // Second parameter (RCX)
    u64 Param3;       // Third parameter (RDX)
    
    // Output (to guest)
    u64 Result;       // Return value (will be in RAX after VMRESUME)
    
    // VCPU context
    VCPU_DATA* Vcpu;
    
} VMCALL_CONTEXT;

// ============================================
// VMCALL Handlers
// ============================================

/**
 * Main VMCALL dispatcher
 * Called from VM exit handler when exit reason is VMCALL
 */
void VmcallDispatch(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_PING
 * Simple test to check if hypervisor is active
 */
u64 VmcallHandlePing(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_GET_INFO
 * Returns hypervisor information
 */
u64 VmcallHandleGetInfo(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_READ_VIRTUAL
 * Read virtual memory from another process
 */
u64 VmcallHandleReadVirtual(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_GET_PROCESS
 * Get process information by PID
 */
u64 VmcallHandleGetProcess(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_GET_MODULE
 * Get module base address
 */
u64 VmcallHandleGetModule(VMCALL_CONTEXT* ctx);

/**
 * Handle VMCALL_READ_GAME_DATA
 * Optimized batch read for game data
 */
u64 VmcallHandleReadGameData(VMCALL_CONTEXT* ctx);

// ============================================
// Process Helpers
// ============================================

/**
 * Find process by name
 * Walks EPROCESS list
 */
u64 VmcallFindProcess(const char* processName, u32* outPid);

/**
 * Get CR3 for process
 */
u64 VmcallGetProcessCr3(u32 pid);

/**
 * Get module base in process
 */
u64 VmcallGetModuleBase(u32 pid, const char* moduleName, u64* outSize);

// ============================================
// Windows Internal Offsets
// ============================================

// These may vary by Windows version
// TODO: Auto-detect or pattern scan

namespace win {

// EPROCESS offsets (Windows 10 22H2)
constexpr u64 EPROCESS_DirectoryTableBase = 0x028;  // CR3
constexpr u64 EPROCESS_UniqueProcessId = 0x440;
constexpr u64 EPROCESS_ActiveProcessLinks = 0x448;
constexpr u64 EPROCESS_ImageFileName = 0x5A8;
constexpr u64 EPROCESS_Peb = 0x550;

// PEB offsets
constexpr u64 PEB_Ldr = 0x18;
constexpr u64 PEB_ImageBaseAddress = 0x10;

// PEB_LDR_DATA offsets
constexpr u64 LDR_InLoadOrderModuleList = 0x10;
constexpr u64 LDR_InMemoryOrderModuleList = 0x20;

// LDR_DATA_TABLE_ENTRY offsets
constexpr u64 LDR_DllBase = 0x30;
constexpr u64 LDR_SizeOfImage = 0x40;
constexpr u64 LDR_FullDllName = 0x48;  // UNICODE_STRING
constexpr u64 LDR_BaseDllName = 0x58;  // UNICODE_STRING

// UNICODE_STRING
constexpr u64 UNICODE_Length = 0x0;
constexpr u64 UNICODE_Buffer = 0x8;

} // namespace win

// ============================================
// Safety Wrappers
// ============================================

/**
 * Safe read from guest virtual memory
 * With full validation
 */
HV_STATUS VmcallSafeRead(
    u64 guestCr3,
    u64 guestVirtual,
    void* hostBuffer,
    u64 size
);

/**
 * Safe read from guest physical memory
 */
HV_STATUS VmcallSafeReadPhysical(
    u64 guestPhysical,
    void* hostBuffer,
    u64 size
);

