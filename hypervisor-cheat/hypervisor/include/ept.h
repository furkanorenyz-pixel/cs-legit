/**
 * HYPERVISOR CHEAT - Extended Page Tables (EPT)
 * Memory virtualization for invisible memory access
 */

#pragma once

#include "../../common/types.h"
#include "safety.h"

// ============================================
// EPT Entry Flags
// ============================================

#define EPT_READ            (1 << 0)
#define EPT_WRITE           (1 << 1)
#define EPT_EXECUTE         (1 << 2)
#define EPT_MEMORY_TYPE     (7 << 3)  // Bits 3-5
#define EPT_IGNORE_PAT      (1 << 6)
#define EPT_LARGE_PAGE      (1 << 7)
#define EPT_ACCESSED        (1 << 8)
#define EPT_DIRTY           (1 << 9)
#define EPT_USER_EXECUTE    (1 << 10)

// Memory types
#define EPT_TYPE_UC         0  // Uncacheable
#define EPT_TYPE_WC         1  // Write-combining
#define EPT_TYPE_WT         4  // Write-through
#define EPT_TYPE_WP         5  // Write-protected
#define EPT_TYPE_WB         6  // Write-back

// Default flags for normal memory
#define EPT_DEFAULT_FLAGS   (EPT_READ | EPT_WRITE | EPT_EXECUTE | (EPT_TYPE_WB << 3))

// ============================================
// EPT Structures
// ============================================

// EPT PML4 Entry (512GB per entry)
typedef union _EPT_PML4E {
    u64 Value;
    struct {
        u64 Read : 1;
        u64 Write : 1;
        u64 Execute : 1;
        u64 Reserved1 : 5;
        u64 Accessed : 1;
        u64 Reserved2 : 1;
        u64 UserExecute : 1;
        u64 Reserved3 : 1;
        u64 PhysicalAddress : 40;  // Bits 12-51
        u64 Reserved4 : 12;
    };
} EPT_PML4E;

// EPT PDPT Entry (1GB per entry)
typedef union _EPT_PDPTE {
    u64 Value;
    struct {
        u64 Read : 1;
        u64 Write : 1;
        u64 Execute : 1;
        u64 Reserved1 : 4;
        u64 LargePage : 1;  // 1GB page if set
        u64 Accessed : 1;
        u64 Reserved2 : 3;
        u64 PhysicalAddress : 40;
        u64 Reserved3 : 12;
    };
} EPT_PDPTE;

// EPT PD Entry (2MB per entry)
typedef union _EPT_PDE {
    u64 Value;
    struct {
        u64 Read : 1;
        u64 Write : 1;
        u64 Execute : 1;
        u64 MemoryType : 3;
        u64 IgnorePat : 1;
        u64 LargePage : 1;  // 2MB page if set
        u64 Accessed : 1;
        u64 Dirty : 1;
        u64 UserExecute : 1;
        u64 Reserved1 : 1;
        u64 PhysicalAddress : 40;
        u64 Reserved2 : 12;
    };
} EPT_PDE;

// EPT PT Entry (4KB per entry)
typedef union _EPT_PTE {
    u64 Value;
    struct {
        u64 Read : 1;
        u64 Write : 1;
        u64 Execute : 1;
        u64 MemoryType : 3;
        u64 IgnorePat : 1;
        u64 Reserved1 : 1;
        u64 Accessed : 1;
        u64 Dirty : 1;
        u64 UserExecute : 1;
        u64 Reserved2 : 1;
        u64 PhysicalAddress : 40;
        u64 Reserved3 : 12;
    };
} EPT_PTE;

// ============================================
// EPT Pointer (EPTP)
// ============================================

typedef union _EPT_POINTER {
    u64 Value;
    struct {
        u64 MemoryType : 3;      // 0 = UC, 6 = WB
        u64 PageWalkLength : 3;  // 3 = 4-level paging
        u64 EnableAccessDirty : 1;
        u64 Reserved1 : 5;
        u64 Pml4Address : 40;    // Physical address of EPT PML4
        u64 Reserved2 : 12;
    };
} EPT_POINTER;

// ============================================
// EPT State
// ============================================

typedef struct _EPT_STATE {
    // Page tables
    EPT_PML4E* Pml4;
    EPT_PDPTE* Pdpt[512];
    EPT_PDE* Pd[512][512];
    
    // Physical addresses
    PHYSICAL_ADDRESS Pml4Phys;
    
    // EPTP value for VMCS
    EPT_POINTER Eptp;
    
    // Stats
    u64 TotalPages;
    u64 MappedPages;
    
} EPT_STATE;

extern EPT_STATE gEptState;

// ============================================
// EPT Functions
// ============================================

/**
 * Initialize EPT with identity mapping
 * Maps all physical memory 1:1
 */
HV_STATUS EptInitialize(void);

/**
 * Shutdown EPT and free resources
 */
void EptShutdown(void);

/**
 * Get EPTP value for VMCS
 */
u64 EptGetPointer(void);

// ============================================
// Memory Access via EPT
// ============================================

/**
 * Read physical memory directly
 * No OS involvement, bypasses all protection
 */
HV_STATUS EptReadPhysical(
    PHYSICAL_ADDRESS physAddr,
    void* buffer,
    u64 size
);

/**
 * Read virtual memory from another process
 * Uses CR3 to translate virtual → physical
 */
HV_STATUS EptReadVirtual(
    u64 targetCr3,
    VIRTUAL_ADDRESS virtAddr,
    void* buffer,
    u64 size
);

/**
 * Translate virtual address to physical
 * Walks page tables of target process
 */
PHYSICAL_ADDRESS EptTranslateVirtual(
    u64 targetCr3,
    VIRTUAL_ADDRESS virtAddr
);

// ============================================
// EPT Hooking (for stealth)
// ============================================

typedef struct _EPT_HOOK {
    PHYSICAL_ADDRESS TargetPhys;
    PHYSICAL_ADDRESS FakePhys;
    void* OriginalPage;
    void* FakePage;
    u32 Active;
} EPT_HOOK;

/**
 * Create EPT hook
 * Shows different memory to reads vs executes
 */
HV_STATUS EptCreateHook(
    PHYSICAL_ADDRESS target,
    void* fakeData,
    u64 size
);

/**
 * Remove EPT hook
 */
HV_STATUS EptRemoveHook(PHYSICAL_ADDRESS target);

// ============================================
// Page Table Walking
// ============================================

// Windows page table structures
typedef union _PML4E_64 {
    u64 Value;
    struct {
        u64 Present : 1;
        u64 Write : 1;
        u64 User : 1;
        u64 WriteThrough : 1;
        u64 CacheDisable : 1;
        u64 Accessed : 1;
        u64 Reserved1 : 1;
        u64 PageSize : 1;  // Must be 0
        u64 Reserved2 : 4;
        u64 PageFrameNumber : 40;
        u64 Reserved3 : 11;
        u64 NoExecute : 1;
    };
} PML4E_64;

typedef union _PDPTE_64 {
    u64 Value;
    struct {
        u64 Present : 1;
        u64 Write : 1;
        u64 User : 1;
        u64 WriteThrough : 1;
        u64 CacheDisable : 1;
        u64 Accessed : 1;
        u64 Dirty : 1;
        u64 PageSize : 1;  // 1 = 1GB page
        u64 Global : 1;
        u64 Reserved1 : 3;
        u64 PageFrameNumber : 40;
        u64 Reserved2 : 11;
        u64 NoExecute : 1;
    };
} PDPTE_64;

typedef union _PDE_64 {
    u64 Value;
    struct {
        u64 Present : 1;
        u64 Write : 1;
        u64 User : 1;
        u64 WriteThrough : 1;
        u64 CacheDisable : 1;
        u64 Accessed : 1;
        u64 Dirty : 1;
        u64 PageSize : 1;  // 1 = 2MB page
        u64 Global : 1;
        u64 Reserved1 : 3;
        u64 PageFrameNumber : 40;
        u64 Reserved2 : 11;
        u64 NoExecute : 1;
    };
} PDE_64;

typedef union _PTE_64 {
    u64 Value;
    struct {
        u64 Present : 1;
        u64 Write : 1;
        u64 User : 1;
        u64 WriteThrough : 1;
        u64 CacheDisable : 1;
        u64 Accessed : 1;
        u64 Dirty : 1;
        u64 Pat : 1;
        u64 Global : 1;
        u64 Reserved1 : 3;
        u64 PageFrameNumber : 40;
        u64 Reserved2 : 11;
        u64 NoExecute : 1;
    };
} PTE_64;

// Page table indices
#define PML4_INDEX(va)  (((va) >> 39) & 0x1FF)
#define PDPT_INDEX(va)  (((va) >> 30) & 0x1FF)
#define PD_INDEX(va)    (((va) >> 21) & 0x1FF)
#define PT_INDEX(va)    (((va) >> 12) & 0x1FF)
#define PAGE_OFFSET(va) ((va) & 0xFFF)

