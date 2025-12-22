/**
 * HYPERVISOR CHEAT - Common Types
 * Shared between bootkit, hypervisor, and usermode
 */

#pragma once

#include <stdint.h>

// ============================================
// Basic Types
// ============================================

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef u64 PHYSICAL_ADDRESS;
typedef u64 VIRTUAL_ADDRESS;

// ============================================
// Status Codes
// ============================================

typedef enum _HV_STATUS {
    HV_SUCCESS = 0,
    
    // Initialization errors
    HV_ERROR_NOT_SUPPORTED = 1,      // CPU doesn't support VT-x
    HV_ERROR_ALREADY_RUNNING = 2,    // Hypervisor already active
    HV_ERROR_VMX_DISABLED = 3,       // VT-x disabled in BIOS
    HV_ERROR_VMXON_FAILED = 4,       // VMXON instruction failed
    HV_ERROR_VMCS_FAILED = 5,        // VMCS setup failed
    HV_ERROR_EPT_FAILED = 6,         // EPT setup failed
    
    // Runtime errors
    HV_ERROR_INVALID_VMCALL = 10,    // Unknown VMCALL number
    HV_ERROR_INVALID_PARAM = 11,     // Bad parameter
    HV_ERROR_INVALID_ADDRESS = 12,   // Invalid memory address
    HV_ERROR_ACCESS_DENIED = 13,     // Access denied
    HV_ERROR_PROCESS_NOT_FOUND = 14, // Process not found
    HV_ERROR_READ_FAILED = 15,       // Memory read failed
    
    // Safety errors
    HV_ERROR_NOT_VM = 20,            // Running on real hardware
    HV_ERROR_SAFETY_TRIGGERED = 21,  // Safety system activated
    HV_ERROR_ROLLBACK_NEEDED = 22,   // Need to rollback
    
} HV_STATUS;

// ============================================
// Safety Configuration
// ============================================

typedef struct _HV_SAFETY_CONFIG {
    u32 VmOnlyMode;          // 1 = Only run in VMs
    u32 DryRunMode;          // 1 = Don't actually do anything
    u32 RollbackOnError;     // 1 = Rollback on any error
    u32 MaxRetries;          // Max operation retries
    u32 LoggingEnabled;      // 1 = Full logging
    u32 SanityChecks;        // 1 = Validate all pointers
} HV_SAFETY_CONFIG;

// Default safe config
#define HV_SAFETY_DEFAULT { \
    .VmOnlyMode = 1,        \
    .DryRunMode = 0,        \
    .RollbackOnError = 1,   \
    .MaxRetries = 3,        \
    .LoggingEnabled = 1,    \
    .SanityChecks = 1       \
}

// ============================================
// VMCALL Request Structures
// ============================================

// Read memory request
typedef struct _HV_READ_REQUEST {
    u64 ProcessCr3;          // Target process CR3
    u64 VirtualAddress;      // Address to read
    u64 BufferAddress;       // Where to store result
    u64 Size;                // Bytes to read
    HV_STATUS Status;        // Result status
} HV_READ_REQUEST;

// Process info request
typedef struct _HV_PROCESS_REQUEST {
    u32 ProcessId;           // Input: PID
    u64 Cr3;                 // Output: CR3
    u64 BaseAddress;         // Output: Main module base
    char Name[64];           // Output: Process name
    HV_STATUS Status;
} HV_PROCESS_REQUEST;

// Module info request
typedef struct _HV_MODULE_REQUEST {
    u32 ProcessId;           // Input: PID
    char ModuleName[64];     // Input: Module name
    u64 BaseAddress;         // Output: Module base
    u64 Size;                // Output: Module size
    HV_STATUS Status;
} HV_MODULE_REQUEST;

// ============================================
// Game Data Structures (CS2)
// ============================================

typedef struct _VECTOR3 {
    float x, y, z;
} VECTOR3;

typedef struct _VECTOR2 {
    float x, y;
} VECTOR2;

typedef struct _PLAYER_DATA {
    u32 Index;
    u32 Health;
    u32 Team;
    u32 IsAlive;
    u32 IsVisible;
    VECTOR3 Position;
    VECTOR3 HeadPosition;
    char Name[32];
} PLAYER_DATA;

typedef struct _GAME_DATA {
    u32 LocalPlayerIndex;
    u32 LocalTeam;
    u32 PlayerCount;
    PLAYER_DATA Players[64];
    float ViewMatrix[16];
} GAME_DATA;

// ============================================
// Hypervisor Info
// ============================================

typedef struct _HV_INFO {
    u32 Version;
    u32 IsActive;
    u32 IsVirtualMachine;
    u64 HypervisorBase;
    u64 HypervisorSize;
    u64 EptBase;
    char Signature[16];      // "HVCHEAT"
} HV_INFO;

#define HV_SIGNATURE "HVCHEAT\0"
#define HV_VERSION   0x00010000  // 1.0.0

// ============================================
// CPU Feature Flags
// ============================================

typedef struct _CPU_FEATURES {
    u32 HasVmx : 1;          // Intel VT-x
    u32 HasSvm : 1;          // AMD-V
    u32 HasEpt : 1;          // Extended Page Tables
    u32 HasVpid : 1;         // Virtual Processor ID
    u32 HasInvEpt : 1;       // INVEPT instruction
    u32 IsVmxEnabled : 1;    // VT-x enabled in BIOS
    u32 IsHypervisorPresent : 1; // Another HV running
    u32 Reserved : 25;
} CPU_FEATURES;

// ============================================
// Macros
// ============================================

#define PAGE_SIZE       0x1000
#define PAGE_MASK       (PAGE_SIZE - 1)
#define PAGE_ALIGN(x)   ((x) & ~PAGE_MASK)
#define PAGE_OFFSET(x)  ((x) & PAGE_MASK)

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

