/**
 * HYPERVISOR CHEAT - VMCALL Codes
 * Communication between usermode and hypervisor
 */

#pragma once

#include <stdint.h>

// ============================================
// VMCALL Numbers
// ============================================

// Magic number to verify our hypervisor
#define VMCALL_MAGIC            0x48564348  // "HVCH"

// System calls
#define VMCALL_PING             0x0001  // Test if HV is active
#define VMCALL_GET_INFO         0x0002  // Get HV info
#define VMCALL_SHUTDOWN         0x0003  // Shutdown HV (debug only)

// Memory operations
#define VMCALL_READ_PHYSICAL    0x0100  // Read physical memory
#define VMCALL_READ_VIRTUAL     0x0101  // Read virtual memory (with CR3)
#define VMCALL_WRITE_PHYSICAL   0x0102  // Write physical memory
#define VMCALL_WRITE_VIRTUAL    0x0103  // Write virtual memory

// Process operations
#define VMCALL_GET_PROCESS      0x0200  // Get process info by PID
#define VMCALL_GET_PROCESS_CR3  0x0201  // Get CR3 for process
#define VMCALL_GET_MODULE       0x0202  // Get module base
#define VMCALL_FIND_PROCESS     0x0203  // Find process by name

// Game-specific (optimized)
#define VMCALL_READ_GAME_DATA   0x0300  // Read all game data at once
#define VMCALL_READ_ENTITY      0x0301  // Read single entity
#define VMCALL_READ_VIEWMATRIX  0x0302  // Read view matrix

// Safety
#define VMCALL_SAFETY_CHECK     0x0F00  // Check safety status
#define VMCALL_ENABLE_SAFE_MODE 0x0F01  // Enable safe mode
#define VMCALL_DISABLE_HV       0x0F02  // Disable all HV features

// ============================================
// VMCALL Response
// ============================================

#define VMCALL_SUCCESS          0x00000000
#define VMCALL_ERROR            0x80000000

// ============================================
// VMCALL Declaration (implemented in vmcall.asm)
// ============================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Execute VMCALL instruction
 * Implemented in assembly (vmcall.asm)
 * 
 * @param number  VMCALL number (will be OR'd with MAGIC << 32)
 * @param param1  First parameter
 * @param param2  Second parameter
 * @param param3  Third parameter
 * @return Result from hypervisor
 */
uint64_t DoVmcall(
    uint64_t number,
    uint64_t param1,
    uint64_t param2,
    uint64_t param3
);

#ifdef __cplusplus
}
#endif

// ============================================
// Helper Functions
// ============================================

#ifdef __cplusplus
// Check if our hypervisor is running
inline bool IsHypervisorActive() {
    uint64_t result = DoVmcall(VMCALL_PING, 0, 0, 0);
    return (result == 0x1);
}
#endif
