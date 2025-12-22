/**
 * HYPERVISOR CHEAT - Safety System Implementation
 * Prevents system damage with comprehensive error handling
 */

#include "../include/safety.h"
#include "../include/vmx.h"
#include <stdarg.h>

// ============================================
// Global State
// ============================================

SAFETY_STATE gSafetyState = {0};
static SAFETY_BACKUP gBackups[MAX_BACKUPS] = {0};

// ============================================
// VM Detection
// ============================================

static void CpuidEx(u32 function, u32 subfunction, 
                    u32* eax, u32* ebx, u32* ecx, u32* edx) {
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(function), "c"(subfunction)
    );
}

u32 SafetyDetectVM(void) {
    u32 eax, ebx, ecx, edx;
    
    // Check CPUID.1:ECX[31] - Hypervisor Present bit
    CpuidEx(1, 0, &eax, &ebx, &ecx, &edx);
    
    if (ecx & (1 << 31)) {
        // Hypervisor present, we're in a VM
        gSafetyState.IsVirtualMachine = 1;
        
        // Get hypervisor vendor
        CpuidEx(0x40000000, 0, &eax, &ebx, &ecx, &edx);
        
        char* vendor = gSafetyState.VmVendor;
        *(u32*)&vendor[0] = ebx;
        *(u32*)&vendor[4] = ecx;
        *(u32*)&vendor[8] = edx;
        vendor[12] = '\0';
        
        // Identify VM type
        if (__builtin_memcmp(vendor, "VMwareVMware", 12) == 0) {
            gSafetyState.VmType = VM_TYPE_VMWARE;
        } else if (__builtin_memcmp(vendor, "VBoxVBoxVBox", 12) == 0) {
            gSafetyState.VmType = VM_TYPE_VIRTUALBOX;
        } else if (__builtin_memcmp(vendor, "KVMKVMKVM", 9) == 0) {
            gSafetyState.VmType = VM_TYPE_KVM;
        } else if (__builtin_memcmp(vendor, "Microsoft Hv", 12) == 0) {
            gSafetyState.VmType = VM_TYPE_HYPERV;
        } else if (__builtin_memcmp(vendor, "TCGTCGTCGTCG", 12) == 0) {
            gSafetyState.VmType = VM_TYPE_QEMU;
        } else if (__builtin_memcmp(vendor, "XenVMMXenVMM", 12) == 0) {
            gSafetyState.VmType = VM_TYPE_XEN;
        } else {
            gSafetyState.VmType = VM_TYPE_UNKNOWN;
        }
        
        return 1;
    }
    
    gSafetyState.IsVirtualMachine = 0;
    gSafetyState.VmType = VM_TYPE_NONE;
    
    return 0;
}

VM_TYPE SafetyGetVmType(void) {
    return gSafetyState.VmType;
}

const char* SafetyGetVmVendor(void) {
    return gSafetyState.VmVendor;
}

// ============================================
// Initialization
// ============================================

HV_STATUS SafetyInit(SAFETY_LEVEL level) {
    // Clear state
    __builtin_memset(&gSafetyState, 0, sizeof(gSafetyState));
    __builtin_memset(gBackups, 0, sizeof(gBackups));
    
    // Set level
    gSafetyState.Level = level;
    
    // Configure based on level
    switch (level) {
        case SAFETY_DISABLED:
            gSafetyState.Config.VmOnlyMode = 0;
            gSafetyState.Config.DryRunMode = 0;
            gSafetyState.Config.RollbackOnError = 0;
            gSafetyState.Config.SanityChecks = 0;
            gSafetyState.Config.LoggingEnabled = 0;
            break;
            
        case SAFETY_MINIMAL:
            gSafetyState.Config.VmOnlyMode = 0;
            gSafetyState.Config.DryRunMode = 0;
            gSafetyState.Config.RollbackOnError = 1;
            gSafetyState.Config.SanityChecks = 0;
            gSafetyState.Config.LoggingEnabled = 1;
            break;
            
        case SAFETY_NORMAL:
            gSafetyState.Config.VmOnlyMode = 1;
            gSafetyState.Config.DryRunMode = 0;
            gSafetyState.Config.RollbackOnError = 1;
            gSafetyState.Config.SanityChecks = 1;
            gSafetyState.Config.LoggingEnabled = 1;
            break;
            
        case SAFETY_PARANOID:
            gSafetyState.Config.VmOnlyMode = 1;
            gSafetyState.Config.DryRunMode = 1;
            gSafetyState.Config.RollbackOnError = 1;
            gSafetyState.Config.SanityChecks = 1;
            gSafetyState.Config.LoggingEnabled = 1;
            break;
    }
    
    gSafetyState.Config.MaxRetries = 3;
    
    // Detect VM
    SafetyDetectVM();
    
    // Check VM-only mode
    if (gSafetyState.Config.VmOnlyMode && !gSafetyState.IsVirtualMachine) {
        LOG_ERROR("╔═══════════════════════════════════════════════════════════╗");
        LOG_ERROR("║  ⚠️  SAFETY BLOCK: NOT RUNNING IN VIRTUAL MACHINE!        ║");
        LOG_ERROR("╠═══════════════════════════════════════════════════════════╣");
        LOG_ERROR("║  Hypervisor will NOT start on real hardware.              ║");
        LOG_ERROR("║  This is for your safety - HV bugs can brick systems!     ║");
        LOG_ERROR("║                                                           ║");
        LOG_ERROR("║  To override (DANGEROUS!):                                ║");
        LOG_ERROR("║    SafetyInit(SAFETY_DISABLED)                            ║");
        LOG_ERROR("╚═══════════════════════════════════════════════════════════╝");
        
        gSafetyState.SafetyTriggered = 1;
        return HV_ERROR_NOT_VM;
    }
    
    gSafetyState.Initialized = 1;
    gSafetyState.BackupData = gBackups;
    
    LOG_INFO("╔═══════════════════════════════════════════════════════════╗");
    LOG_INFO("║  Safety System Initialized                                ║");
    LOG_INFO("╠═══════════════════════════════════════════════════════════╣");
    LOG_INFO("║  Level: %s", 
        level == SAFETY_DISABLED ? "DISABLED (DANGEROUS!)" :
        level == SAFETY_MINIMAL ? "MINIMAL" :
        level == SAFETY_NORMAL ? "NORMAL" : "PARANOID");
    LOG_INFO("║  VM Only: %s", gSafetyState.Config.VmOnlyMode ? "YES" : "NO");
    LOG_INFO("║  Dry Run: %s", gSafetyState.Config.DryRunMode ? "YES" : "NO");
    LOG_INFO("║  Auto Rollback: %s", gSafetyState.Config.RollbackOnError ? "YES" : "NO");
    LOG_INFO("║  Environment: %s (%s)", 
        gSafetyState.IsVirtualMachine ? "Virtual Machine" : "Real Hardware",
        gSafetyState.VmVendor[0] ? gSafetyState.VmVendor : "N/A");
    LOG_INFO("╚═══════════════════════════════════════════════════════════╝");
    
    return HV_SUCCESS;
}

void SafetyShutdown(void) {
    // Rollback any pending changes
    if (gSafetyState.Config.RollbackOnError) {
        SafetyRollbackAll();
    }
    
    gSafetyState.Initialized = 0;
    LOG_INFO("[Safety] Shutdown complete");
}

u32 SafetyCheck(void) {
    if (!gSafetyState.Initialized) {
        return 0;
    }
    
    if (gSafetyState.SafetyTriggered) {
        return 0;
    }
    
    if (gSafetyState.ErrorCount >= gSafetyState.Config.MaxRetries) {
        LOG_ERROR("[Safety] Too many errors (%u), triggering safety shutdown",
                  gSafetyState.ErrorCount);
        gSafetyState.SafetyTriggered = 1;
        return 0;
    }
    
    return 1;
}

// ============================================
// Error Handling
// ============================================

void SafetyReportError(HV_STATUS error, const char* message) {
    gSafetyState.LastError = error;
    gSafetyState.ErrorCount++;
    
    LOG_ERROR("[Safety] Error %u: %s", error, message ? message : "Unknown");
    
    if (gSafetyState.ErrorCount >= gSafetyState.Config.MaxRetries) {
        LOG_ERROR("[Safety] Max errors reached, initiating rollback...");
        
        if (gSafetyState.Config.RollbackOnError) {
            SafetyRollbackAll();
        }
        
        gSafetyState.SafetyTriggered = 1;
    }
}

HV_STATUS SafetyGetLastError(void) {
    return gSafetyState.LastError;
}

void SafetyClearErrors(void) {
    gSafetyState.LastError = HV_SUCCESS;
    gSafetyState.ErrorCount = 0;
}

// ============================================
// Pointer Validation
// ============================================

u32 SafetyValidatePointer(void* ptr, u64 size) {
    if (!gSafetyState.Config.SanityChecks) {
        return 1;  // Checks disabled
    }
    
    if (ptr == NULL) {
        return 0;
    }
    
    u64 addr = (u64)ptr;
    
    // Check for obviously bad addresses
    if (addr < 0x1000) {
        return 0;  // Null page
    }
    
    // Check for non-canonical addresses (x64)
    if (addr > 0x00007FFFFFFFFFFF && addr < 0xFFFF800000000000) {
        return 0;
    }
    
    // Check for overflow
    if (addr + size < addr) {
        return 0;
    }
    
    return 1;
}

u32 SafetyValidatePhysical(PHYSICAL_ADDRESS addr, u64 size) {
    if (!gSafetyState.Config.SanityChecks) {
        return 1;
    }
    
    // Check for reasonable physical address
    // Most systems have < 1TB RAM
    if (addr > 0x10000000000ULL) {  // 1TB
        return 0;
    }
    
    if (addr + size < addr) {
        return 0;
    }
    
    return 1;
}

u32 SafetyValidateVirtual(VIRTUAL_ADDRESS addr, u64 size) {
    return SafetyValidatePointer((void*)addr, size);
}

// ============================================
// Backup & Rollback
// ============================================

HV_STATUS SafetyCreateBackup(void* address, u64 size) {
    if (gSafetyState.BackupCount >= MAX_BACKUPS) {
        LOG_WARN("[Safety] Backup slots full!");
        return HV_ERROR_SAFETY_TRIGGERED;
    }
    
    if (size > sizeof(gBackups[0].OriginalData)) {
        LOG_WARN("[Safety] Backup size too large: %lu", size);
        return HV_ERROR_INVALID_PARAM;
    }
    
    if (!SafetyValidatePointer(address, size)) {
        return HV_ERROR_INVALID_ADDRESS;
    }
    
    SAFETY_BACKUP* backup = &gBackups[gSafetyState.BackupCount];
    backup->Address = address;
    backup->Size = size;
    backup->Applied = 0;
    
    // Copy original data
    __builtin_memcpy(backup->OriginalData, address, size);
    
    gSafetyState.BackupCount++;
    
    LOG_DEBUG("[Safety] Backup created: %p (%lu bytes)", address, size);
    
    return HV_SUCCESS;
}

HV_STATUS SafetyRollbackOne(u32 index) {
    if (index >= gSafetyState.BackupCount) {
        return HV_ERROR_INVALID_PARAM;
    }
    
    SAFETY_BACKUP* backup = &gBackups[index];
    
    if (backup->Applied && backup->Address != NULL) {
        __builtin_memcpy(backup->Address, backup->OriginalData, backup->Size);
        backup->Applied = 0;
        LOG_INFO("[Safety] Rolled back: %p", backup->Address);
    }
    
    return HV_SUCCESS;
}

HV_STATUS SafetyRollbackAll(void) {
    LOG_WARN("[Safety] ════════════════════════════════════════");
    LOG_WARN("[Safety] ROLLING BACK ALL MODIFICATIONS");
    LOG_WARN("[Safety] ════════════════════════════════════════");
    
    u32 rolledBack = 0;
    
    for (u32 i = 0; i < gSafetyState.BackupCount; i++) {
        SAFETY_BACKUP* backup = &gBackups[i];
        
        if (backup->Applied && backup->Address != NULL) {
            __builtin_memcpy(backup->Address, backup->OriginalData, backup->Size);
            backup->Applied = 0;
            rolledBack++;
        }
    }
    
    LOG_INFO("[Safety] Rolled back %u modifications", rolledBack);
    
    return HV_SUCCESS;
}

HV_STATUS SafetyCommitBackup(u32 index) {
    if (index >= gSafetyState.BackupCount) {
        return HV_ERROR_INVALID_PARAM;
    }
    
    gBackups[index].Applied = 1;
    return HV_SUCCESS;
}

// ============================================
// Safe Memory Operations
// ============================================

HV_STATUS SafeMemoryCopy(void* dest, const void* src, u64 size) {
    if (!SafetyValidatePointer(dest, size)) {
        return HV_ERROR_INVALID_ADDRESS;
    }
    
    if (!SafetyValidatePointer((void*)src, size)) {
        return HV_ERROR_INVALID_ADDRESS;
    }
    
    __builtin_memcpy(dest, src, size);
    return HV_SUCCESS;
}

HV_STATUS SafeMemorySet(void* dest, u8 value, u64 size) {
    if (!SafetyValidatePointer(dest, size)) {
        return HV_ERROR_INVALID_ADDRESS;
    }
    
    __builtin_memset(dest, value, size);
    return HV_SUCCESS;
}

i32 SafeMemoryCompare(const void* a, const void* b, u64 size) {
    return __builtin_memcmp(a, b, size);
}

// ============================================
// Logging
// ============================================

static const char* gLogLevelNames[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

void SafetyLog(LOG_LEVEL level, const char* format, ...) {
    if (!gSafetyState.Config.LoggingEnabled && level < LOG_ERROR) {
        return;
    }
    
    // In real implementation, this would write to serial/debug port
    // For now, just a stub
    (void)level;
    (void)format;
    
    // TODO: Implement actual logging
    // Options:
    // - Write to serial port (COM1)
    // - Write to debug buffer
    // - Use firmware console (if available)
}

