/*
 * KERNEL BYPASS FRAMEWORK - Ring -1 Hypervisor
 * Intel VT-x (VMX) Definitions and API
 */

#pragma once

#include <ntddk.h>
#include "../../common/types.h"
#include "ia32.h"

// ============================================
// VMX Operation Result
// ============================================

typedef enum _VMX_STATUS {
    VMX_SUCCESS = 0,
    VMX_ERROR_WITH_STATUS = 1,
    VMX_ERROR_WITHOUT_STATUS = 2
} VMX_STATUS;

// ============================================
// Virtual CPU State
// ============================================

typedef struct _VCPU {
    // Host state
    PVOID HostStack;
    ULONG64 HostStackSize;
    PVOID HostStateArea;    // For saving host state on VM exit
    
    // VMXON region
    PVOID VmxonRegion;
    PHYSICAL_ADDRESS VmxonRegionPhys;
    
    // VMCS region
    PVOID VmcsRegion;
    PHYSICAL_ADDRESS VmcsRegionPhys;
    
    // EPT structures
    PVOID EptPml4;
    PHYSICAL_ADDRESS EptPml4Phys;
    
    // State
    BOOLEAN VmxEnabled;
    BOOLEAN VmcsActive;
    ULONG CpuIndex;
    
    // Saved guest state
    ULONG64 GuestRip;
    ULONG64 GuestRsp;
    ULONG64 GuestRflags;
    CONTEXT GuestContext;
    
} VCPU, *PVCPU;

// ============================================
// Global Hypervisor State
// ============================================

typedef struct _HYPERVISOR_CONTEXT {
    PVCPU Vcpus;
    ULONG VcpuCount;
    BOOLEAN Initialized;
    BOOLEAN Running;
    
    // EPT
    PVOID EptState;
    
    // Hook list
    struct {
        ULONG64 TargetAddress;
        ULONG64 HookAddress;
        BOOLEAN Active;
    } EptHooks[256];
    ULONG EptHookCount;
    
    // CPUID spoofing
    struct {
        ULONG Function;
        ULONG Eax, Ebx, Ecx, Edx;
        BOOLEAN Active;
    } CpuidSpoof[16];
    ULONG CpuidSpoofCount;
    
} HYPERVISOR_CONTEXT, *PHYPERVISOR_CONTEXT;

extern HYPERVISOR_CONTEXT g_HypervisorContext;

// ============================================
// Initialization
// ============================================

/*
 * Check if CPU supports VT-x
 */
BOOLEAN VmxIsSupported(VOID);

/*
 * Initialize hypervisor on all CPUs
 */
NTSTATUS VmxInitialize(VOID);

/*
 * Shutdown hypervisor on all CPUs
 */
NTSTATUS VmxShutdown(VOID);

/*
 * Get hypervisor status
 */
BOOLEAN VmxIsRunning(VOID);

// ============================================
// Per-CPU Operations
// ============================================

/*
 * Initialize VMX on current CPU
 */
NTSTATUS VmxInitializeCpu(
    _In_ ULONG CpuIndex,
    _Out_ PVCPU Vcpu
);

/*
 * Enable VMX operation (VMXON)
 */
NTSTATUS VmxEnable(
    _Inout_ PVCPU Vcpu
);

/*
 * Setup VMCS
 */
NTSTATUS VmxSetupVmcs(
    _Inout_ PVCPU Vcpu
);

/*
 * Launch guest (VMLAUNCH)
 */
NTSTATUS VmxLaunchGuest(
    _Inout_ PVCPU Vcpu
);

/*
 * Resume guest (VMRESUME)
 */
NTSTATUS VmxResumeGuest(
    _Inout_ PVCPU Vcpu
);

/*
 * Disable VMX (VMXOFF)
 */
NTSTATUS VmxDisable(
    _Inout_ PVCPU Vcpu
);

// ============================================
// VMCS Operations
// ============================================

/*
 * Read VMCS field
 */
ULONG64 VmxRead(
    _In_ ULONG Field
);

/*
 * Write VMCS field
 */
VMX_STATUS VmxWrite(
    _In_ ULONG Field,
    _In_ ULONG64 Value
);

// ============================================
// VM Exit Handler
// ============================================

/*
 * Main VM Exit handler
 * Called from assembly stub
 */
BOOLEAN VmxExitHandler(
    _Inout_ PVCPU Vcpu
);

// ============================================
// EPT (Extended Page Tables)
// ============================================

/*
 * Initialize EPT
 */
NTSTATUS VmxSetupEpt(
    _Inout_ PVCPU Vcpu
);

/*
 * Create EPT hook (execute-only page)
 */
NTSTATUS VmxEptHook(
    _In_ ULONG64 TargetAddress,
    _In_ ULONG64 HookFunction,
    _Out_ PULONG64 OriginalBytes
);

/*
 * Remove EPT hook
 */
NTSTATUS VmxEptUnhook(
    _In_ ULONG64 TargetAddress
);

// ============================================
// CPUID Spoofing
// ============================================

/*
 * Add CPUID spoof entry
 */
NTSTATUS VmxSpoofCpuid(
    _In_ ULONG Function,
    _In_ ULONG Eax,
    _In_ ULONG Ebx,
    _In_ ULONG Ecx,
    _In_ ULONG Edx
);

/*
 * Remove CPUID spoof
 */
NTSTATUS VmxUnspoofCpuid(
    _In_ ULONG Function
);

// ============================================
// Hypercall Interface
// ============================================

#define VMCALL_MAGIC            0x4B42564D  // "KBVM"

// Hypercall numbers
#define VMCALL_TEST             0x0001
#define VMCALL_READ_MEMORY      0x0002
#define VMCALL_WRITE_MEMORY     0x0003
#define VMCALL_HIDE_PAGE        0x0004
#define VMCALL_UNHIDE_PAGE      0x0005
#define VMCALL_EPT_HOOK         0x0006
#define VMCALL_EPT_UNHOOK       0x0007
#define VMCALL_GET_STATUS       0x00FF

/*
 * Execute hypercall from guest
 */
ULONG64 VmxVmcall(
    _In_ ULONG64 VmcallNumber,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2,
    _In_ ULONG64 Param3
);

// ============================================
// Assembly Functions (defined in vmx_asm.asm)
// ============================================

extern VOID AsmVmxSaveState(VOID);
extern VOID AsmVmxRestoreState(VOID);
extern VOID AsmVmxLaunch(VOID);
extern VOID AsmVmxResume(VOID);
extern VOID AsmVmxExitHandler(VOID);
extern ULONG64 AsmVmxVmcall(ULONG64 Magic, ULONG64 VmcallNumber, ULONG64 Param1, ULONG64 Param2, ULONG64 Param3);

