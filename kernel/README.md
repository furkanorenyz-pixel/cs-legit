# 🔥 KERNEL BYPASS FRAMEWORK v1.0.0

## Universal Multi-Ring Bypass Library

```
┌─────────────────────────────────────────────────────────────────┐
│                    RING LEVELS ARCHITECTURE                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   USER APPLICATION                                               │
│        ↓                                                         │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  USERMODE LIBRARY (libkernel.lib)                       │   │
│   │  • Unified API for all ring levels                      │   │
│   │  • Auto-detection of available backend                  │   │
│   │  • Fallback chain: R-3 → R-1 → R0 → R3                 │   │
│   └─────────────────────────────────────────────────────────┘   │
│        ↓                                                         │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  RING 0 - KERNEL DRIVER (driver.sys)                    │   │
│   │  • MmCopyVirtualMemory                                  │   │
│   │  • DKOM (Direct Kernel Object Manipulation)             │   │
│   │  • Callback removal                                      │   │
│   │  • HWID Spoofer                                          │   │
│   └─────────────────────────────────────────────────────────┘   │
│        ↓                                                         │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  RING -1 - HYPERVISOR (hypervisor.sys)                  │   │
│   │  • Intel VT-x / AMD-V                                   │   │
│   │  • EPT Hooking                                           │   │
│   │  • VM Exit Handler                                       │   │
│   │  • Invisible to OS                                       │   │
│   └─────────────────────────────────────────────────────────┘   │
│        ↓                                                         │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  RING -2/-3 - FIRMWARE (smm.efi / me_exploit)           │   │
│   │  • SMM Handler                                           │   │
│   │  • UEFI Runtime Services                                 │   │
│   │  • Intel ME Exploitation                                 │   │
│   │  • Hardware-level persistence                            │   │
│   └─────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
kernel/
├── 📂 ring0/                    # Ring 0 - Kernel Driver
│   ├── driver/                  # Main driver source
│   │   ├── main.c              # DriverEntry
│   │   ├── memory.c            # Memory read/write
│   │   ├── process.c           # Process utilities
│   │   ├── callbacks.c         # Callback manipulation
│   │   ├── hwid.c              # HWID spoofing
│   │   └── ioctl.h             # IOCTL definitions
│   ├── mapper/                  # Driver loader/mapper
│   │   ├── kdmapper.cpp        # Kernel driver mapper
│   │   └── vulnerable_drivers/ # Exploitable drivers
│   └── CMakeLists.txt
│
├── 📂 ring_minus1/              # Ring -1 - Hypervisor
│   ├── hypervisor/
│   │   ├── vmx.c               # Intel VT-x setup
│   │   ├── ept.c               # Extended Page Tables
│   │   ├── vmexit.c            # VM Exit handler
│   │   ├── vmcall.c            # Hypercall interface
│   │   └── svm.c               # AMD-V (optional)
│   ├── include/
│   │   ├── ia32.h              # Intel architecture defs
│   │   └── vmx.h               # VMX structures
│   └── CMakeLists.txt
│
├── 📂 ring_minus2/              # Ring -2 - SMM
│   ├── smm/
│   │   ├── smm_handler.c       # SMM interrupt handler
│   │   └── smm_comm.c          # Communication channel
│   └── CMakeLists.txt
│
├── 📂 ring_minus3/              # Ring -3 - Intel ME / Firmware
│   ├── uefi/
│   │   ├── runtime_driver.c    # UEFI runtime DXE driver
│   │   ├── bootkit.c           # Boot-level persistence
│   │   └── secure_boot.c       # Secure Boot bypass
│   ├── me/
│   │   └── me_exploit.c        # Intel ME exploitation
│   └── CMakeLists.txt
│
├── 📂 usermode/                 # User-mode interface library
│   ├── include/
│   │   ├── kernel_interface.h  # Main API header
│   │   ├── memory.h            # Memory operations
│   │   ├── process.h           # Process operations
│   │   └── hwid.h              # HWID operations
│   ├── src/
│   │   ├── interface.cpp       # Unified interface
│   │   ├── driver_comm.cpp     # Ring 0 communication
│   │   ├── hyper_comm.cpp      # Ring -1 communication
│   │   └── firmware_comm.cpp   # Ring -2/-3 communication
│   └── CMakeLists.txt
│
├── 📂 common/                   # Shared definitions
│   ├── types.h                 # Common types
│   ├── ioctl_codes.h           # IOCTL definitions
│   └── status.h                # Status codes
│
├── 📂 tools/                    # Development tools
│   ├── driver_loader/          # Manual driver loader
│   ├── debug_console/          # Kernel debug viewer
│   └── signature_bypass/       # DSE bypass tools
│
├── 📂 docs/                     # Documentation
│   ├── RING0.md                # Kernel driver guide
│   ├── HYPERVISOR.md           # VT-x guide
│   ├── FIRMWARE.md             # UEFI/SMM guide
│   └── ANTI_DETECTION.md       # Evasion techniques
│
├── CMakeLists.txt              # Root build file
└── README.md                   # This file
```

---

## 🎯 Features by Ring Level

### Ring 0 - Kernel Driver
| Feature | Status | Description |
|---------|--------|-------------|
| Memory Read/Write | 🔄 | MmCopyVirtualMemory |
| Process Hide | 🔄 | DKOM - unlink from EPROCESS list |
| Handle Elevation | 🔄 | Grant PROCESS_ALL_ACCESS |
| Callback Remove | 🔄 | Disable anticheat callbacks |
| HWID Spoof | 🔄 | Disk, MAC, SMBIOS, GPU |
| Driver Hide | 🔄 | Unlink from DriverObject list |

### Ring -1 - Hypervisor
| Feature | Status | Description |
|---------|--------|-------------|
| VT-x Setup | 🔄 | VMXON, VMCS configuration |
| EPT Hook | 🔄 | Invisible code hooks |
| Syscall Hook | 🔄 | Intercept kernel calls |
| Memory Hide | 🔄 | Hide pages from OS |
| Anti-Debug | 🔄 | Detect/block debuggers |

### Ring -2 - SMM
| Feature | Status | Description |
|---------|--------|-------------|
| SMI Handler | 🔄 | System Management Interrupt |
| Hidden Memory | 🔄 | SMRAM access |
| Persistence | 🔄 | Survives OS reinstall |

### Ring -3 - Firmware
| Feature | Status | Description |
|---------|--------|-------------|
| UEFI Driver | 🔄 | Boot-level code execution |
| Bootkit | 🔄 | Pre-OS code execution |
| ME Exploit | 🔄 | Intel ME vulnerabilities |
| Secure Boot Bypass | 🔄 | Load unsigned code |

---

## 🔧 Build Requirements

### Ring 0 (Kernel Driver)
```
✅ Visual Studio 2022
✅ Windows Driver Kit (WDK) 10/11
✅ Windows SDK
✅ Test signing enabled OR EV certificate
```

### Ring -1 (Hypervisor)
```
✅ Intel CPU with VT-x OR AMD CPU with AMD-V
✅ BIOS: VT-x/AMD-V enabled
✅ Windows 10/11 (Hyper-V disabled)
```

### Ring -2/-3 (Firmware)
```
✅ EDK2 (UEFI Development Kit)
✅ Intel ME SDK (for ME exploitation)
✅ SPI programmer (for firmware flashing)
✅ Test hardware (DO NOT USE ON MAIN PC!)
```

---

## 🚀 Quick Start

### 1. Build Kernel Driver
```bash
cd kernel/ring0
cmake -G "Visual Studio 17 2022" -A x64 -B build
cmake --build build --config Release
```

### 2. Load Driver (Test Mode)
```bash
# Enable test signing (run as admin, reboot required)
bcdedit /set testsigning on

# Load driver
sc create KernelBypass binPath= "C:\path\to\driver.sys" type= kernel
sc start KernelBypass
```

### 3. Use from Usermode
```cpp
#include <kernel_interface.h>

int main() {
    KernelInterface ki;
    
    // Auto-detect best available backend
    if (!ki.Initialize()) {
        printf("Failed to initialize kernel interface\n");
        return 1;
    }
    
    // Read memory from any process
    DWORD pid = GetCS2ProcessId();
    uintptr_t value = ki.Read<uintptr_t>(pid, address);
    
    // Spoof HWID
    ki.SpoofDiskSerial("FAKE1234");
    ki.SpoofMAC({0x00, 0x11, 0x22, 0x33, 0x44, 0x55});
    
    return 0;
}
```

---

## ⚠️ Safety Guidelines

1. **ALWAYS test in Virtual Machine first!**
2. **NEVER run untested kernel code on main system!**
3. **Keep backups of BIOS/UEFI before firmware mods!**
4. **Use separate test hardware for Ring -2/-3!**

---

## 📚 Learning Resources

### Ring 0
- Windows Internals (Russinovich)
- Windows Kernel Programming (Pavel Yosifovich)
- OSR Online - Driver Development

### Ring -1
- Intel SDM Volume 3 (VMX)
- Hypervisor From Scratch (Sina Karvandi)
- hvpp - Minimalistic hypervisor

### Ring -2/-3
- UEFI Specification
- Intel ME documentation
- Firmware Security (McAfee research)

---

## 📜 License

Educational purposes only. Use responsibly.

---

**Made for universal anti-cheat bypass research** 🔥

