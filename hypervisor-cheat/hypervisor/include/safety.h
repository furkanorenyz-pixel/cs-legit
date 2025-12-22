/**
 * HYPERVISOR CHEAT - Safety System
 * Prevents system damage and provides rollback capability
 */

#pragma once

#include "../../common/types.h"

// ============================================
// Safety Levels
// ============================================

typedef enum _SAFETY_LEVEL {
    SAFETY_DISABLED = 0,     // No safety (DANGEROUS!)
    SAFETY_MINIMAL = 1,      // Basic checks only
    SAFETY_NORMAL = 2,       // Standard safety
    SAFETY_PARANOID = 3,     // Maximum safety (VM only, dry run)
} SAFETY_LEVEL;

// ============================================
// Safety State
// ============================================

typedef struct _SAFETY_STATE {
    // Configuration
    HV_SAFETY_CONFIG Config;
    SAFETY_LEVEL Level;
    
    // Runtime state
    u32 Initialized;
    u32 SafetyTriggered;
    u32 ErrorCount;
    u32 LastError;
    
    // Environment detection
    u32 IsVirtualMachine;
    u32 VmType;              // 0=unknown, 1=VMware, 2=VBox, 3=KVM, etc.
    char VmVendor[16];
    
    // Rollback data
    u32 BackupCount;
    void* BackupData;        // Array of backups
    
} SAFETY_STATE;

// Global safety state
extern SAFETY_STATE gSafetyState;

// ============================================
// VM Detection
// ============================================

typedef enum _VM_TYPE {
    VM_TYPE_NONE = 0,        // Real hardware
    VM_TYPE_VMWARE = 1,
    VM_TYPE_VIRTUALBOX = 2,
    VM_TYPE_KVM = 3,
    VM_TYPE_HYPERV = 4,
    VM_TYPE_QEMU = 5,
    VM_TYPE_XEN = 6,
    VM_TYPE_UNKNOWN = 255,
} VM_TYPE;

/**
 * Detect if running in VM
 * Uses CPUID and other techniques
 */
u32 SafetyDetectVM(void);

/**
 * Get VM type
 */
VM_TYPE SafetyGetVmType(void);

/**
 * Get VM vendor string
 */
const char* SafetyGetVmVendor(void);

// ============================================
// Initialization
// ============================================

/**
 * Initialize safety system
 * Must be called before any hypervisor operations
 */
HV_STATUS SafetyInit(SAFETY_LEVEL level);

/**
 * Shutdown safety system
 */
void SafetyShutdown(void);

/**
 * Check if safe to continue
 * Returns 0 if should abort
 */
u32 SafetyCheck(void);

// ============================================
// Error Handling
// ============================================

/**
 * Report an error
 * If too many errors, triggers safety shutdown
 */
void SafetyReportError(HV_STATUS error, const char* message);

/**
 * Get last error
 */
HV_STATUS SafetyGetLastError(void);

/**
 * Clear error state
 */
void SafetyClearErrors(void);

// ============================================
// Pointer Validation
// ============================================

/**
 * Validate a pointer before use
 */
u32 SafetyValidatePointer(void* ptr, u64 size);

/**
 * Validate a physical address
 */
u32 SafetyValidatePhysical(PHYSICAL_ADDRESS addr, u64 size);

/**
 * Validate a virtual address
 */
u32 SafetyValidateVirtual(VIRTUAL_ADDRESS addr, u64 size);

// ============================================
// Backup & Rollback
// ============================================

typedef struct _SAFETY_BACKUP {
    void* Address;
    u8 OriginalData[256];
    u8 NewData[256];
    u64 Size;
    u32 Applied;
} SAFETY_BACKUP;

#define MAX_BACKUPS 32

/**
 * Create backup before modification
 */
HV_STATUS SafetyCreateBackup(void* address, u64 size);

/**
 * Rollback single backup
 */
HV_STATUS SafetyRollbackOne(u32 index);

/**
 * Rollback ALL modifications
 * Called on any critical error
 */
HV_STATUS SafetyRollbackAll(void);

/**
 * Commit a backup (mark as applied)
 */
HV_STATUS SafetyCommitBackup(u32 index);

// ============================================
// Safe Operations
// ============================================

/**
 * Safe memory copy with validation
 */
HV_STATUS SafeMemoryCopy(void* dest, const void* src, u64 size);

/**
 * Safe memory set with validation
 */
HV_STATUS SafeMemorySet(void* dest, u8 value, u64 size);

/**
 * Safe memory compare
 */
i32 SafeMemoryCompare(const void* a, const void* b, u64 size);

// ============================================
// Logging
// ============================================

typedef enum _LOG_LEVEL {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4,
} LOG_LEVEL;

/**
 * Log a message
 */
void SafetyLog(LOG_LEVEL level, const char* format, ...);

#define LOG_DEBUG(fmt, ...) SafetyLog(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  SafetyLog(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  SafetyLog(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) SafetyLog(LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) SafetyLog(LOG_FATAL, fmt, ##__VA_ARGS__)

// ============================================
// Assertions
// ============================================

#define SAFETY_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            SafetyReportError(HV_ERROR_SAFETY_TRIGGERED, message); \
            return HV_ERROR_SAFETY_TRIGGERED; \
        } \
    } while(0)

#define SAFETY_ASSERT_POINTER(ptr, size) \
    SAFETY_ASSERT(SafetyValidatePointer((void*)(ptr), (size)), "Invalid pointer")

#define SAFETY_ASSERT_VM() \
    SAFETY_ASSERT(gSafetyState.IsVirtualMachine || !gSafetyState.Config.VmOnlyMode, \
                  "Not running in VM!")

