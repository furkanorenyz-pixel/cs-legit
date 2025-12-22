/**
 * HYPERVISOR CHEAT - Intel VT-x Definitions
 * Core hypervisor structures and functions
 */

#pragma once

#include "../../common/types.h"
#include "safety.h"

// ============================================
// VMX MSRs
// ============================================

#define MSR_IA32_FEATURE_CONTROL        0x3A
#define MSR_IA32_VMX_BASIC              0x480
#define MSR_IA32_VMX_PINBASED_CTLS      0x481
#define MSR_IA32_VMX_PROCBASED_CTLS     0x482
#define MSR_IA32_VMX_EXIT_CTLS          0x483
#define MSR_IA32_VMX_ENTRY_CTLS         0x484
#define MSR_IA32_VMX_MISC               0x485
#define MSR_IA32_VMX_CR0_FIXED0         0x486
#define MSR_IA32_VMX_CR0_FIXED1         0x487
#define MSR_IA32_VMX_CR4_FIXED0         0x488
#define MSR_IA32_VMX_CR4_FIXED1         0x489
#define MSR_IA32_VMX_PROCBASED_CTLS2    0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP       0x48C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS 0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS     0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS    0x490

// ============================================
// VMCS Fields
// ============================================

// 16-bit control fields
#define VMCS_VIRTUAL_PROCESSOR_ID       0x0000
#define VMCS_POSTED_INT_NOTIFY_VECTOR   0x0002

// 16-bit guest state fields
#define VMCS_GUEST_ES_SELECTOR          0x0800
#define VMCS_GUEST_CS_SELECTOR          0x0802
#define VMCS_GUEST_SS_SELECTOR          0x0804
#define VMCS_GUEST_DS_SELECTOR          0x0806
#define VMCS_GUEST_FS_SELECTOR          0x0808
#define VMCS_GUEST_GS_SELECTOR          0x080A
#define VMCS_GUEST_LDTR_SELECTOR        0x080C
#define VMCS_GUEST_TR_SELECTOR          0x080E

// 64-bit control fields
#define VMCS_IO_BITMAP_A                0x2000
#define VMCS_IO_BITMAP_B                0x2002
#define VMCS_MSR_BITMAP                 0x2004
#define VMCS_EPT_POINTER                0x201A

// 64-bit guest state
#define VMCS_GUEST_VMCS_LINK_POINTER    0x2800

// 32-bit control fields
#define VMCS_PIN_BASED_VM_EXEC_CONTROL  0x4000
#define VMCS_CPU_BASED_VM_EXEC_CONTROL  0x4002
#define VMCS_EXCEPTION_BITMAP           0x4004
#define VMCS_VM_EXIT_CONTROLS           0x400C
#define VMCS_VM_ENTRY_CONTROLS          0x4012
#define VMCS_SECONDARY_VM_EXEC_CONTROL  0x401E

// 32-bit guest state
#define VMCS_GUEST_ES_LIMIT             0x4800
#define VMCS_GUEST_CS_LIMIT             0x4802
#define VMCS_GUEST_SS_LIMIT             0x4804
#define VMCS_GUEST_DS_LIMIT             0x4806
#define VMCS_GUEST_FS_LIMIT             0x4808
#define VMCS_GUEST_GS_LIMIT             0x480A
#define VMCS_GUEST_LDTR_LIMIT           0x480C
#define VMCS_GUEST_TR_LIMIT             0x480E
#define VMCS_GUEST_GDTR_LIMIT           0x4810
#define VMCS_GUEST_IDTR_LIMIT           0x4812
#define VMCS_GUEST_INTERRUPTIBILITY     0x4824
#define VMCS_GUEST_ACTIVITY_STATE       0x4826

// Natural-width guest state
#define VMCS_GUEST_CR0                  0x6800
#define VMCS_GUEST_CR3                  0x6802
#define VMCS_GUEST_CR4                  0x6804
#define VMCS_GUEST_ES_BASE              0x6806
#define VMCS_GUEST_CS_BASE              0x6808
#define VMCS_GUEST_SS_BASE              0x680A
#define VMCS_GUEST_DS_BASE              0x680C
#define VMCS_GUEST_FS_BASE              0x680E
#define VMCS_GUEST_GS_BASE              0x6810
#define VMCS_GUEST_LDTR_BASE            0x6812
#define VMCS_GUEST_TR_BASE              0x6814
#define VMCS_GUEST_GDTR_BASE            0x6816
#define VMCS_GUEST_IDTR_BASE            0x6818
#define VMCS_GUEST_DR7                  0x681A
#define VMCS_GUEST_RSP                  0x681C
#define VMCS_GUEST_RIP                  0x681E
#define VMCS_GUEST_RFLAGS               0x6820

// Natural-width host state
#define VMCS_HOST_CR0                   0x6C00
#define VMCS_HOST_CR3                   0x6C02
#define VMCS_HOST_CR4                   0x6C04
#define VMCS_HOST_RSP                   0x6C14
#define VMCS_HOST_RIP                   0x6C16

// ============================================
// VM Exit Reasons
// ============================================

#define EXIT_REASON_EXCEPTION_NMI       0
#define EXIT_REASON_EXTERNAL_INTERRUPT  1
#define EXIT_REASON_TRIPLE_FAULT        2
#define EXIT_REASON_CPUID               10
#define EXIT_REASON_RDTSC               16
#define EXIT_REASON_VMCALL              18
#define EXIT_REASON_VMCLEAR             19
#define EXIT_REASON_VMLAUNCH            20
#define EXIT_REASON_VMREAD              23
#define EXIT_REASON_VMWRITE             24
#define EXIT_REASON_CR_ACCESS           28
#define EXIT_REASON_MSR_READ            31
#define EXIT_REASON_MSR_WRITE           32
#define EXIT_REASON_EPT_VIOLATION       48
#define EXIT_REASON_EPT_MISCONFIG       49
#define EXIT_REASON_RDTSCP              51
#define EXIT_REASON_XSETBV              55

// ============================================
// Control Flags
// ============================================

// Pin-based controls
#define PIN_BASED_EXT_INTR_EXIT         (1 << 0)
#define PIN_BASED_NMI_EXIT              (1 << 3)
#define PIN_BASED_VIRTUAL_NMIS          (1 << 5)

// Primary processor-based controls
#define CPU_BASED_HLT_EXIT              (1 << 7)
#define CPU_BASED_INVLPG_EXIT           (1 << 9)
#define CPU_BASED_MWAIT_EXIT            (1 << 10)
#define CPU_BASED_RDPMC_EXIT            (1 << 11)
#define CPU_BASED_RDTSC_EXIT            (1 << 12)
#define CPU_BASED_CR3_LOAD_EXIT         (1 << 15)
#define CPU_BASED_CR3_STORE_EXIT        (1 << 16)
#define CPU_BASED_MOV_DR_EXIT           (1 << 23)
#define CPU_BASED_USE_IO_BITMAPS        (1 << 25)
#define CPU_BASED_USE_MSR_BITMAPS       (1 << 28)
#define CPU_BASED_MONITOR_EXIT          (1 << 29)
#define CPU_BASED_PAUSE_EXIT            (1 << 30)
#define CPU_BASED_ACTIVATE_SECONDARY    (1 << 31)

// Secondary processor-based controls
#define CPU_BASED_2_ENABLE_EPT          (1 << 1)
#define CPU_BASED_2_RDTSCP_EXIT         (1 << 3)
#define CPU_BASED_2_ENABLE_VPID         (1 << 5)
#define CPU_BASED_2_UNRESTRICTED_GUEST  (1 << 7)
#define CPU_BASED_2_ENABLE_INVPCID      (1 << 12)
#define CPU_BASED_2_ENABLE_XSAVES       (1 << 20)

// VM-exit controls
#define VM_EXIT_HOST_ADDR_SPACE_SIZE    (1 << 9)
#define VM_EXIT_ACK_INTR_ON_EXIT        (1 << 15)
#define VM_EXIT_SAVE_IA32_EFER          (1 << 20)
#define VM_EXIT_LOAD_IA32_EFER          (1 << 21)

// VM-entry controls
#define VM_ENTRY_IA32E_MODE             (1 << 9)
#define VM_ENTRY_LOAD_IA32_EFER         (1 << 15)

// ============================================
// Per-CPU Data
// ============================================

typedef struct _VCPU_DATA {
    // VMX regions (must be page-aligned)
    void* VmxonRegion;
    void* VmcsRegion;
    void* MsrBitmap;
    void* HostStack;
    
    // Physical addresses
    PHYSICAL_ADDRESS VmxonRegionPhys;
    PHYSICAL_ADDRESS VmcsRegionPhys;
    PHYSICAL_ADDRESS MsrBitmapPhys;
    
    // State
    u32 CpuIndex;
    u32 IsVmxEnabled;
    u32 IsVmcsActive;
    
    // Guest state saved during VM exit
    u64 GuestRax;
    u64 GuestRbx;
    u64 GuestRcx;
    u64 GuestRdx;
    u64 GuestRsi;
    u64 GuestRdi;
    u64 GuestRbp;
    u64 GuestR8;
    u64 GuestR9;
    u64 GuestR10;
    u64 GuestR11;
    u64 GuestR12;
    u64 GuestR13;
    u64 GuestR14;
    u64 GuestR15;
    
} VCPU_DATA;

// ============================================
// Global Hypervisor Data
// ============================================

typedef struct _HV_GLOBAL_DATA {
    // Safety
    SAFETY_STATE Safety;
    
    // CPU data (per logical processor)
    u32 CpuCount;
    VCPU_DATA* VcpuData;  // Array[CpuCount]
    
    // EPT
    void* EptPml4;
    PHYSICAL_ADDRESS EptPml4Phys;
    
    // State
    u32 Initialized;
    u32 Running;
    
    // Info
    HV_INFO Info;
    
} HV_GLOBAL_DATA;

extern HV_GLOBAL_DATA gHvData;

// ============================================
// VMX Functions
// ============================================

/**
 * Check if CPU supports VT-x
 */
HV_STATUS VmxCheckSupport(CPU_FEATURES* features);

/**
 * Enable VMX operation on current CPU
 */
HV_STATUS VmxEnable(void);

/**
 * Disable VMX operation
 */
HV_STATUS VmxDisable(void);

/**
 * Setup VMCS for current CPU
 */
HV_STATUS VmxSetupVmcs(VCPU_DATA* vcpu);

/**
 * Launch VM (enter VMX non-root)
 */
HV_STATUS VmxLaunch(void);

/**
 * Handle VM exit
 */
void VmxHandleExit(VCPU_DATA* vcpu);

// ============================================
// VMX Intrinsics
// ============================================

static inline u64 __vmx_vmread(u64 field) {
    u64 value;
    __asm__ volatile("vmread %1, %0" : "=r"(value) : "r"(field) : "cc");
    return value;
}

static inline void __vmx_vmwrite(u64 field, u64 value) {
    __asm__ volatile("vmwrite %1, %0" :: "r"(field), "r"(value) : "cc");
}

static inline u8 __vmx_vmxon(u64* vmxon_region) {
    u8 error;
    __asm__ volatile(
        "vmxon %1; setna %0"
        : "=q"(error)
        : "m"(*vmxon_region)
        : "cc", "memory"
    );
    return error;
}

static inline void __vmx_vmxoff(void) {
    __asm__ volatile("vmxoff" ::: "cc");
}

static inline u8 __vmx_vmclear(u64* vmcs) {
    u8 error;
    __asm__ volatile(
        "vmclear %1; setna %0"
        : "=q"(error)
        : "m"(*vmcs)
        : "cc", "memory"
    );
    return error;
}

static inline u8 __vmx_vmptrld(u64* vmcs) {
    u8 error;
    __asm__ volatile(
        "vmptrld %1; setna %0"
        : "=q"(error)
        : "m"(*vmcs)
        : "cc", "memory"
    );
    return error;
}

static inline u8 __vmx_vmlaunch(void) {
    u8 error;
    __asm__ volatile(
        "vmlaunch; setna %0"
        : "=q"(error)
        :: "cc", "memory"
    );
    return error;
}

static inline u8 __vmx_vmresume(void) {
    u8 error;
    __asm__ volatile(
        "vmresume; setna %0"
        : "=q"(error)
        :: "cc", "memory"
    );
    return error;
}

