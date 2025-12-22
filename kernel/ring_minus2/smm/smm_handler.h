/*
 * KERNEL BYPASS FRAMEWORK - Ring -2 SMM
 * System Management Mode Handler
 * 
 * ⚠️ WARNING: SMM code runs at Ring -2, below the OS
 * Improper implementation can brick hardware!
 * Test ONLY on dedicated hardware with BIOS recovery option!
 */

#pragma once

// SMM runs without OS, use basic types
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef UINT64              UINTN;

// ============================================
// SMM Communication
// ============================================

/*
 * SMM Communication Buffer
 * Shared between OS and SMM handler
 */
typedef struct _SMM_COMM_BUFFER {
    UINT32 Magic;           // SMMC (0x434D4D53)
    UINT32 Command;         // Operation to perform
    UINT32 Status;          // Return status
    UINT32 Reserved;
    UINT64 Param1;
    UINT64 Param2;
    UINT64 Param3;
    UINT64 ReturnValue;
    UINT8  Data[4096 - 48]; // Additional data
} SMM_COMM_BUFFER, *PSMM_COMM_BUFFER;

#define SMM_MAGIC   0x434D4D53  // "SMMC"

// SMM Commands
typedef enum _SMM_COMMAND {
    SMM_CMD_PING            = 0x0001,
    SMM_CMD_READ_PHYS       = 0x0002,
    SMM_CMD_WRITE_PHYS      = 0x0003,
    SMM_CMD_READ_MSR        = 0x0004,
    SMM_CMD_WRITE_MSR       = 0x0005,
    SMM_CMD_READ_IO         = 0x0006,
    SMM_CMD_WRITE_IO        = 0x0007,
    SMM_CMD_EXECUTE         = 0x0008,
    SMM_CMD_HIDE_MEMORY     = 0x0009,
    SMM_CMD_SPOOF_SMBIOS    = 0x000A,
} SMM_COMMAND;

// SMM Status codes
typedef enum _SMM_STATUS {
    SMM_SUCCESS             = 0,
    SMM_ERROR_INVALID_CMD   = 1,
    SMM_ERROR_INVALID_PARAM = 2,
    SMM_ERROR_ACCESS_DENIED = 3,
    SMM_ERROR_NOT_FOUND     = 4,
} SMM_STATUS;

// ============================================
// SMM Save State (CPU state when entering SMM)
// ============================================

// Offset from SMBASE
#define SMM_SAVE_STATE_OFFSET   0xFC00

typedef struct _SMM_SAVE_STATE {
    // This is architecture-specific
    // Intel and AMD have different layouts
    UINT8 Reserved1[0x7E00];
    
    // At offset 0x7E00 from SMBASE
    UINT64 GdtBase;
    UINT64 LdtBase;
    UINT64 IdtBase;
    UINT64 Tr;
    
    UINT64 IoRestart;
    UINT64 IoInstructionRestart;
    UINT64 AutoHaltRestart;
    
    // General purpose registers
    UINT64 R15;
    UINT64 R14;
    UINT64 R13;
    UINT64 R12;
    UINT64 R11;
    UINT64 R10;
    UINT64 R9;
    UINT64 R8;
    UINT64 Rdi;
    UINT64 Rsi;
    UINT64 Rbp;
    UINT64 Rsp;
    UINT64 Rbx;
    UINT64 Rdx;
    UINT64 Rcx;
    UINT64 Rax;
    
    // Control registers
    UINT64 Cr0;
    UINT64 Cr3;
    UINT64 Cr4;
    UINT64 Efer;
    
    // Segment registers
    UINT64 Cs;
    UINT64 Ss;
    UINT64 Ds;
    UINT64 Es;
    UINT64 Fs;
    UINT64 Gs;
    
    UINT64 Rip;
    UINT64 Rflags;
    UINT64 Dr6;
    UINT64 Dr7;
    
} SMM_SAVE_STATE, *PSMM_SAVE_STATE;

// ============================================
// SMM Handler Functions
// ============================================

/*
 * Main SMI (System Management Interrupt) handler
 * Called when SMI is triggered (via port 0xB2 or other)
 */
void SmmHandler(void);

/*
 * Initialize SMM handler
 * Called during BIOS POST to install our handler
 */
SMM_STATUS SmmInstallHandler(void);

/*
 * Process command from OS
 */
SMM_STATUS SmmProcessCommand(PSMM_COMM_BUFFER Buffer);

// ============================================
// SMM Operations
// ============================================

/*
 * Read physical memory from SMM
 * Has access to ALL memory including SMRAM
 */
SMM_STATUS SmmReadPhysical(
    UINT64 PhysicalAddress,
    UINT64 Buffer,
    UINT64 Size
);

/*
 * Write physical memory from SMM
 */
SMM_STATUS SmmWritePhysical(
    UINT64 PhysicalAddress,
    UINT64 Buffer,
    UINT64 Size
);

/*
 * Hide memory region from OS
 * Uses SMRAM or TSEG manipulation
 */
SMM_STATUS SmmHideMemory(
    UINT64 PhysicalAddress,
    UINT64 Size
);

/*
 * Read MSR (no restrictions in SMM)
 */
UINT64 SmmReadMsr(UINT32 Msr);

/*
 * Write MSR (no restrictions in SMM)
 */
void SmmWriteMsr(UINT32 Msr, UINT64 Value);

// ============================================
// Triggering SMI from OS
// ============================================

/*
 * Trigger SMI from Ring 0 kernel driver
 * Writes to I/O port 0xB2 (APM_CNT)
 */
#define SMI_PORT        0xB2
#define SMI_COMMAND     0x42    // Custom SMI command code

// In kernel driver:
// __outbyte(SMI_PORT, SMI_COMMAND);

