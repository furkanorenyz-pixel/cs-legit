/*
 * KERNEL BYPASS FRAMEWORK - Ring -1 Hypervisor
 * Intel Architecture Definitions (IA-32/Intel 64)
 * Based on Intel SDM Volume 3
 */

#pragma once

#include <ntddk.h>

// ============================================
// MSR (Model Specific Registers)
// ============================================

#define MSR_IA32_FEATURE_CONTROL        0x0000003A
#define MSR_IA32_VMX_BASIC              0x00000480
#define MSR_IA32_VMX_PINBASED_CTLS      0x00000481
#define MSR_IA32_VMX_PROCBASED_CTLS     0x00000482
#define MSR_IA32_VMX_EXIT_CTLS          0x00000483
#define MSR_IA32_VMX_ENTRY_CTLS         0x00000484
#define MSR_IA32_VMX_MISC               0x00000485
#define MSR_IA32_VMX_CR0_FIXED0         0x00000486
#define MSR_IA32_VMX_CR0_FIXED1         0x00000487
#define MSR_IA32_VMX_CR4_FIXED0         0x00000488
#define MSR_IA32_VMX_CR4_FIXED1         0x00000489
#define MSR_IA32_VMX_PROCBASED_CTLS2    0x0000048B
#define MSR_IA32_VMX_EPT_VPID_CAP       0x0000048C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS 0x0000048D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x0000048E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS     0x0000048F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS    0x00000490

#define MSR_IA32_SYSENTER_CS            0x00000174
#define MSR_IA32_SYSENTER_ESP           0x00000175
#define MSR_IA32_SYSENTER_EIP           0x00000176
#define MSR_IA32_DEBUGCTL               0x000001D9
#define MSR_IA32_PAT                    0x00000277
#define MSR_IA32_EFER                   0xC0000080
#define MSR_IA32_STAR                   0xC0000081
#define MSR_IA32_LSTAR                  0xC0000082
#define MSR_IA32_CSTAR                  0xC0000083
#define MSR_IA32_FMASK                  0xC0000084
#define MSR_IA32_FS_BASE                0xC0000100
#define MSR_IA32_GS_BASE                0xC0000101
#define MSR_IA32_KERNEL_GS_BASE         0xC0000102

// ============================================
// CPUID Leaves
// ============================================

#define CPUID_VMX_BIT                   (1 << 5)   // ECX bit 5 for CPUID.1
#define CPUID_HYPERVISOR_BIT            (1 << 31)  // ECX bit 31 for CPUID.1

// ============================================
// CR Bits
// ============================================

#define CR0_PE                          (1 << 0)
#define CR0_MP                          (1 << 1)
#define CR0_EM                          (1 << 2)
#define CR0_TS                          (1 << 3)
#define CR0_ET                          (1 << 4)
#define CR0_NE                          (1 << 5)
#define CR0_WP                          (1 << 16)
#define CR0_AM                          (1 << 18)
#define CR0_NW                          (1 << 29)
#define CR0_CD                          (1 << 30)
#define CR0_PG                          (1 << 31)

#define CR4_VME                         (1 << 0)
#define CR4_PVI                         (1 << 1)
#define CR4_TSD                         (1 << 2)
#define CR4_DE                          (1 << 3)
#define CR4_PSE                         (1 << 4)
#define CR4_PAE                         (1 << 5)
#define CR4_MCE                         (1 << 6)
#define CR4_PGE                         (1 << 7)
#define CR4_PCE                         (1 << 8)
#define CR4_OSFXSR                      (1 << 9)
#define CR4_OSXMMEXCPT                  (1 << 10)
#define CR4_VMXE                        (1 << 13)
#define CR4_SMXE                        (1 << 14)
#define CR4_FSGSBASE                    (1 << 16)
#define CR4_PCIDE                       (1 << 17)
#define CR4_OSXSAVE                     (1 << 18)

// ============================================
// VMCS Field Encodings
// ============================================

// 16-bit control fields
#define VMCS_CTRL_VPID                          0x00000000
#define VMCS_CTRL_POSTED_INTR_NOTIFY_VECTOR     0x00000002
#define VMCS_CTRL_EPTP_INDEX                    0x00000004

// 16-bit guest state
#define VMCS_GUEST_ES_SEL                       0x00000800
#define VMCS_GUEST_CS_SEL                       0x00000802
#define VMCS_GUEST_SS_SEL                       0x00000804
#define VMCS_GUEST_DS_SEL                       0x00000806
#define VMCS_GUEST_FS_SEL                       0x00000808
#define VMCS_GUEST_GS_SEL                       0x0000080A
#define VMCS_GUEST_LDTR_SEL                     0x0000080C
#define VMCS_GUEST_TR_SEL                       0x0000080E
#define VMCS_GUEST_INTR_STATUS                  0x00000810

// 16-bit host state
#define VMCS_HOST_ES_SEL                        0x00000C00
#define VMCS_HOST_CS_SEL                        0x00000C02
#define VMCS_HOST_SS_SEL                        0x00000C04
#define VMCS_HOST_DS_SEL                        0x00000C06
#define VMCS_HOST_FS_SEL                        0x00000C08
#define VMCS_HOST_GS_SEL                        0x00000C0A
#define VMCS_HOST_TR_SEL                        0x00000C0C

// 64-bit control fields
#define VMCS_CTRL_IO_BITMAP_A                   0x00002000
#define VMCS_CTRL_IO_BITMAP_B                   0x00002002
#define VMCS_CTRL_MSR_BITMAP                    0x00002004
#define VMCS_CTRL_VMEXIT_MSR_STORE              0x00002006
#define VMCS_CTRL_VMEXIT_MSR_LOAD               0x00002008
#define VMCS_CTRL_VMENTRY_MSR_LOAD              0x0000200A
#define VMCS_CTRL_EXECUTIVE_VMCS_PTR            0x0000200C
#define VMCS_CTRL_TSC_OFFSET                    0x00002010
#define VMCS_CTRL_VIRTUAL_APIC                  0x00002012
#define VMCS_CTRL_APIC_ACCESS                   0x00002014
#define VMCS_CTRL_POSTED_INTR_DESC              0x00002016
#define VMCS_CTRL_VMFUNC_CONTROLS               0x00002018
#define VMCS_CTRL_EPTP                          0x0000201A
#define VMCS_CTRL_EOI_EXIT_BITMAP_0             0x0000201C
#define VMCS_CTRL_EOI_EXIT_BITMAP_1             0x0000201E
#define VMCS_CTRL_EOI_EXIT_BITMAP_2             0x00002020
#define VMCS_CTRL_EOI_EXIT_BITMAP_3             0x00002022
#define VMCS_CTRL_EPTP_LIST                     0x00002024
#define VMCS_CTRL_VMREAD_BITMAP                 0x00002026
#define VMCS_CTRL_VMWRITE_BITMAP                0x00002028
#define VMCS_CTRL_XSS_EXITING_BITMAP            0x0000202C

// 64-bit guest state
#define VMCS_GUEST_VMCS_LINK_PTR                0x00002800
#define VMCS_GUEST_DEBUGCTL                     0x00002802
#define VMCS_GUEST_PAT                          0x00002804
#define VMCS_GUEST_EFER                         0x00002806
#define VMCS_GUEST_PERF_GLOBAL_CTRL             0x00002808
#define VMCS_GUEST_PDPTE0                       0x0000280A
#define VMCS_GUEST_PDPTE1                       0x0000280C
#define VMCS_GUEST_PDPTE2                       0x0000280E
#define VMCS_GUEST_PDPTE3                       0x00002810

// 64-bit host state
#define VMCS_HOST_PAT                           0x00002C00
#define VMCS_HOST_EFER                          0x00002C02
#define VMCS_HOST_PERF_GLOBAL_CTRL              0x00002C04

// 32-bit control fields
#define VMCS_CTRL_PIN_BASED                     0x00004000
#define VMCS_CTRL_PROCESSOR_BASED               0x00004002
#define VMCS_CTRL_EXCEPTION_BITMAP              0x00004004
#define VMCS_CTRL_PAGEFAULT_ERROR_MASK          0x00004006
#define VMCS_CTRL_PAGEFAULT_ERROR_MATCH         0x00004008
#define VMCS_CTRL_CR3_TARGET_COUNT              0x0000400A
#define VMCS_CTRL_VMEXIT                        0x0000400C
#define VMCS_CTRL_VMEXIT_MSR_STORE_COUNT        0x0000400E
#define VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT         0x00004010
#define VMCS_CTRL_VMENTRY                       0x00004012
#define VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT        0x00004014
#define VMCS_CTRL_VMENTRY_INTERRUPTION_INFO     0x00004016
#define VMCS_CTRL_VMENTRY_EXCEPTION_ERRORCODE   0x00004018
#define VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH    0x0000401A
#define VMCS_CTRL_TPR_THRESHOLD                 0x0000401C
#define VMCS_CTRL_SECONDARY_PROCESSOR_BASED     0x0000401E
#define VMCS_CTRL_PLE_GAP                       0x00004020
#define VMCS_CTRL_PLE_WINDOW                    0x00004022

// 32-bit read-only fields
#define VMCS_EXIT_REASON                        0x00004402
#define VMCS_EXIT_QUALIFICATION                 0x00004404 // Actually 64-bit
#define VMCS_EXIT_INTERRUPTION_INFO             0x00004404
#define VMCS_EXIT_INTERRUPTION_ERRORCODE        0x00004406
#define VMCS_EXIT_IDT_VECTORING_INFO            0x00004408
#define VMCS_EXIT_IDT_VECTORING_ERRORCODE       0x0000440A
#define VMCS_EXIT_INSTRUCTION_LENGTH            0x0000440C
#define VMCS_EXIT_INSTRUCTION_INFO              0x0000440E

// 32-bit guest state
#define VMCS_GUEST_ES_LIMIT                     0x00004800
#define VMCS_GUEST_CS_LIMIT                     0x00004802
#define VMCS_GUEST_SS_LIMIT                     0x00004804
#define VMCS_GUEST_DS_LIMIT                     0x00004806
#define VMCS_GUEST_FS_LIMIT                     0x00004808
#define VMCS_GUEST_GS_LIMIT                     0x0000480A
#define VMCS_GUEST_LDTR_LIMIT                   0x0000480C
#define VMCS_GUEST_TR_LIMIT                     0x0000480E
#define VMCS_GUEST_GDTR_LIMIT                   0x00004810
#define VMCS_GUEST_IDTR_LIMIT                   0x00004812
#define VMCS_GUEST_ES_ACCESS_RIGHTS             0x00004814
#define VMCS_GUEST_CS_ACCESS_RIGHTS             0x00004816
#define VMCS_GUEST_SS_ACCESS_RIGHTS             0x00004818
#define VMCS_GUEST_DS_ACCESS_RIGHTS             0x0000481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS             0x0000481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS             0x0000481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS           0x00004820
#define VMCS_GUEST_TR_ACCESS_RIGHTS             0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY             0x00004824
#define VMCS_GUEST_ACTIVITY_STATE               0x00004826
#define VMCS_GUEST_SMBASE                       0x00004828
#define VMCS_GUEST_SYSENTER_CS                  0x0000482A
#define VMCS_GUEST_PREEMPTION_TIMER             0x0000482E

// 32-bit host state
#define VMCS_HOST_SYSENTER_CS                   0x00004C00

// Natural-width control fields
#define VMCS_CTRL_CR0_GUEST_HOST_MASK           0x00006000
#define VMCS_CTRL_CR4_GUEST_HOST_MASK           0x00006002
#define VMCS_CTRL_CR0_READ_SHADOW               0x00006004
#define VMCS_CTRL_CR4_READ_SHADOW               0x00006006
#define VMCS_CTRL_CR3_TARGET_VALUE_0            0x00006008
#define VMCS_CTRL_CR3_TARGET_VALUE_1            0x0000600A
#define VMCS_CTRL_CR3_TARGET_VALUE_2            0x0000600C
#define VMCS_CTRL_CR3_TARGET_VALUE_3            0x0000600E

// Natural-width read-only fields
#define VMCS_EXIT_QUALIFICATION_NAT             0x00006400
#define VMCS_IO_RCX                             0x00006402
#define VMCS_IO_RSI                             0x00006404
#define VMCS_IO_RDI                             0x00006406
#define VMCS_IO_RIP                             0x00006408
#define VMCS_GUEST_LINEAR_ADDRESS               0x0000640A

// Natural-width guest state
#define VMCS_GUEST_CR0                          0x00006800
#define VMCS_GUEST_CR3                          0x00006802
#define VMCS_GUEST_CR4                          0x00006804
#define VMCS_GUEST_ES_BASE                      0x00006806
#define VMCS_GUEST_CS_BASE                      0x00006808
#define VMCS_GUEST_SS_BASE                      0x0000680A
#define VMCS_GUEST_DS_BASE                      0x0000680C
#define VMCS_GUEST_FS_BASE                      0x0000680E
#define VMCS_GUEST_GS_BASE                      0x00006810
#define VMCS_GUEST_LDTR_BASE                    0x00006812
#define VMCS_GUEST_TR_BASE                      0x00006814
#define VMCS_GUEST_GDTR_BASE                    0x00006816
#define VMCS_GUEST_IDTR_BASE                    0x00006818
#define VMCS_GUEST_DR7                          0x0000681A
#define VMCS_GUEST_RSP                          0x0000681C
#define VMCS_GUEST_RIP                          0x0000681E
#define VMCS_GUEST_RFLAGS                       0x00006820
#define VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS     0x00006822
#define VMCS_GUEST_SYSENTER_ESP                 0x00006824
#define VMCS_GUEST_SYSENTER_EIP                 0x00006826

// Natural-width host state
#define VMCS_HOST_CR0                           0x00006C00
#define VMCS_HOST_CR3                           0x00006C02
#define VMCS_HOST_CR4                           0x00006C04
#define VMCS_HOST_FS_BASE                       0x00006C06
#define VMCS_HOST_GS_BASE                       0x00006C08
#define VMCS_HOST_TR_BASE                       0x00006C0A
#define VMCS_HOST_GDTR_BASE                     0x00006C0C
#define VMCS_HOST_IDTR_BASE                     0x00006C0E
#define VMCS_HOST_SYSENTER_ESP                  0x00006C10
#define VMCS_HOST_SYSENTER_EIP                  0x00006C12
#define VMCS_HOST_RSP                           0x00006C14
#define VMCS_HOST_RIP                           0x00006C16

// ============================================
// VM Exit Reasons
// ============================================

#define VMX_EXIT_REASON_EXCEPTION_NMI           0
#define VMX_EXIT_REASON_EXTERNAL_INTERRUPT      1
#define VMX_EXIT_REASON_TRIPLE_FAULT            2
#define VMX_EXIT_REASON_INIT                    3
#define VMX_EXIT_REASON_SIPI                    4
#define VMX_EXIT_REASON_IO_SMI                  5
#define VMX_EXIT_REASON_OTHER_SMI               6
#define VMX_EXIT_REASON_INTERRUPT_WINDOW        7
#define VMX_EXIT_REASON_NMI_WINDOW              8
#define VMX_EXIT_REASON_TASK_SWITCH             9
#define VMX_EXIT_REASON_CPUID                   10
#define VMX_EXIT_REASON_GETSEC                  11
#define VMX_EXIT_REASON_HLT                     12
#define VMX_EXIT_REASON_INVD                    13
#define VMX_EXIT_REASON_INVLPG                  14
#define VMX_EXIT_REASON_RDPMC                   15
#define VMX_EXIT_REASON_RDTSC                   16
#define VMX_EXIT_REASON_RSM                     17
#define VMX_EXIT_REASON_VMCALL                  18
#define VMX_EXIT_REASON_VMCLEAR                 19
#define VMX_EXIT_REASON_VMLAUNCH                20
#define VMX_EXIT_REASON_VMPTRLD                 21
#define VMX_EXIT_REASON_VMPTRST                 22
#define VMX_EXIT_REASON_VMREAD                  23
#define VMX_EXIT_REASON_VMRESUME                24
#define VMX_EXIT_REASON_VMWRITE                 25
#define VMX_EXIT_REASON_VMXOFF                  26
#define VMX_EXIT_REASON_VMXON                   27
#define VMX_EXIT_REASON_CR_ACCESS               28
#define VMX_EXIT_REASON_DR_ACCESS               29
#define VMX_EXIT_REASON_IO_INSTRUCTION          30
#define VMX_EXIT_REASON_MSR_READ                31
#define VMX_EXIT_REASON_MSR_WRITE               32
#define VMX_EXIT_REASON_INVALID_GUEST_STATE     33
#define VMX_EXIT_REASON_MSR_LOADING             34
#define VMX_EXIT_REASON_MWAIT                   36
#define VMX_EXIT_REASON_MONITOR_TRAP_FLAG       37
#define VMX_EXIT_REASON_MONITOR                 39
#define VMX_EXIT_REASON_PAUSE                   40
#define VMX_EXIT_REASON_MCE_DURING_ENTRY        41
#define VMX_EXIT_REASON_TPR_BELOW_THRESHOLD     43
#define VMX_EXIT_REASON_APIC_ACCESS             44
#define VMX_EXIT_REASON_EOI_INDUCED             45
#define VMX_EXIT_REASON_GDTR_IDTR               46
#define VMX_EXIT_REASON_LDTR_TR                 47
#define VMX_EXIT_REASON_EPT_VIOLATION           48
#define VMX_EXIT_REASON_EPT_MISCONFIG           49
#define VMX_EXIT_REASON_INVEPT                  50
#define VMX_EXIT_REASON_RDTSCP                  51
#define VMX_EXIT_REASON_PREEMPTION_TIMER        52
#define VMX_EXIT_REASON_INVVPID                 53
#define VMX_EXIT_REASON_WBINVD                  54
#define VMX_EXIT_REASON_XSETBV                  55
#define VMX_EXIT_REASON_APIC_WRITE              56
#define VMX_EXIT_REASON_RDRAND                  57
#define VMX_EXIT_REASON_INVPCID                 58
#define VMX_EXIT_REASON_VMFUNC                  59
#define VMX_EXIT_REASON_ENCLS                   60
#define VMX_EXIT_REASON_RDSEED                  61
#define VMX_EXIT_REASON_PML_FULL                62
#define VMX_EXIT_REASON_XSAVES                  63
#define VMX_EXIT_REASON_XRSTORS                 64

// ============================================
// EPT Structures
// ============================================

// EPT PML4 Entry
typedef union _EPT_PML4E {
    ULONG64 Value;
    struct {
        ULONG64 Read : 1;
        ULONG64 Write : 1;
        ULONG64 Execute : 1;
        ULONG64 Reserved1 : 5;
        ULONG64 Accessed : 1;
        ULONG64 Ignored1 : 1;
        ULONG64 ExecuteForUserMode : 1;
        ULONG64 Ignored2 : 1;
        ULONG64 PhysicalAddress : 40;
        ULONG64 Ignored3 : 12;
    };
} EPT_PML4E, *PEPT_PML4E;

// EPT PDPT Entry
typedef union _EPT_PDPTE {
    ULONG64 Value;
    struct {
        ULONG64 Read : 1;
        ULONG64 Write : 1;
        ULONG64 Execute : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 LargePage : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 ExecuteForUserMode : 1;
        ULONG64 Ignored1 : 1;
        ULONG64 PhysicalAddress : 40;
        ULONG64 Ignored2 : 11;
        ULONG64 SuppressVe : 1;
    };
} EPT_PDPTE, *PEPT_PDPTE;

// EPT PD Entry
typedef union _EPT_PDE {
    ULONG64 Value;
    struct {
        ULONG64 Read : 1;
        ULONG64 Write : 1;
        ULONG64 Execute : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 LargePage : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 ExecuteForUserMode : 1;
        ULONG64 Ignored1 : 1;
        ULONG64 PhysicalAddress : 40;
        ULONG64 Ignored2 : 11;
        ULONG64 SuppressVe : 1;
    };
} EPT_PDE, *PEPT_PDE;

// EPT PT Entry
typedef union _EPT_PTE {
    ULONG64 Value;
    struct {
        ULONG64 Read : 1;
        ULONG64 Write : 1;
        ULONG64 Execute : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 Ignored1 : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 ExecuteForUserMode : 1;
        ULONG64 Ignored2 : 1;
        ULONG64 PhysicalAddress : 40;
        ULONG64 Ignored3 : 11;
        ULONG64 SuppressVe : 1;
    };
} EPT_PTE, *PEPT_PTE;

// EPT Pointer
typedef union _EPTP {
    ULONG64 Value;
    struct {
        ULONG64 MemoryType : 3;
        ULONG64 PageWalkLength : 3;  // Must be 3 (4-level)
        ULONG64 DirtyAndAccessEnabled : 1;
        ULONG64 Reserved1 : 5;
        ULONG64 PhysicalAddress : 40;
        ULONG64 Reserved2 : 12;
    };
} EPTP, *PEPTP;

// ============================================
// Segment Descriptor
// ============================================

typedef struct _SEGMENT_DESCRIPTOR {
    USHORT LimitLow;
    USHORT BaseLow;
    union {
        struct {
            UCHAR BaseMiddle;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHigh;
        } Bytes;
        struct {
            ULONG BaseMid : 8;
            ULONG Type : 4;
            ULONG S : 1;
            ULONG Dpl : 2;
            ULONG Present : 1;
            ULONG LimitHigh : 4;
            ULONG Avl : 1;
            ULONG L : 1;
            ULONG Db : 1;
            ULONG Gran : 1;
            ULONG BaseHigh : 8;
        } Bits;
    };
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

// ============================================
// Inline Assembly Helpers
// ============================================

__forceinline ULONG64 __readmsr(ULONG msr) {
    return __readmsr(msr);
}

__forceinline VOID __writemsr(ULONG msr, ULONG64 value) {
    __writemsr(msr, value);
}

__forceinline ULONG64 __readcr0(VOID) {
    return __readcr0();
}

__forceinline ULONG64 __readcr3(VOID) {
    return __readcr3();
}

__forceinline ULONG64 __readcr4(VOID) {
    return __readcr4();
}

__forceinline VOID __writecr0(ULONG64 value) {
    __writecr0(value);
}

__forceinline VOID __writecr4(ULONG64 value) {
    __writecr4(value);
}

