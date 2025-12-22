/**
 * HYPERVISOR CHEAT - VMCALL Codes
 * Communication between usermode and hypervisor
 */

#pragma once

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
// Inline VMCALL (for usermode)
// ============================================

#ifdef _MSC_VER
#include <intrin.h>

// VMCALL wrapper - works even from Ring 3 if HV is active
static __forceinline unsigned __int64 DoVmcall(
    unsigned __int64 number,
    unsigned __int64 param1,
    unsigned __int64 param2,
    unsigned __int64 param3
) {
    unsigned __int64 result;
    
    // VMCALL with magic number in high bits
    // RAX = VMCALL number | (MAGIC << 32)
    // RBX = param1
    // RCX = param2
    // RDX = param3
    
    __asm {
        mov rax, number
        mov rbx, param1
        mov rcx, param2
        mov rdx, param3
        
        ; VMCALL instruction
        ; db 0x0F, 0x01, 0xC1
        vmcall
        
        mov result, rax
    }
    
    return result;
}

#else  // GCC/Clang

static inline unsigned long long DoVmcall(
    unsigned long long number,
    unsigned long long param1,
    unsigned long long param2,
    unsigned long long param3
) {
    unsigned long long result;
    
    __asm__ volatile(
        "vmcall"
        : "=a"(result)
        : "a"(number | ((unsigned long long)VMCALL_MAGIC << 32)),
          "b"(param1),
          "c"(param2),
          "d"(param3)
        : "memory"
    );
    
    return result;
}

#endif

// ============================================
// Helper Functions
// ============================================

// Check if our hypervisor is running
static inline int IsHypervisorActive(void) {
    unsigned long long result = DoVmcall(VMCALL_PING, 0, 0, 0);
    return (result == 0x1);
}

