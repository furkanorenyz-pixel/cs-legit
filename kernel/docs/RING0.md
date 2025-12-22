# 🔧 Ring 0 - Kernel Driver Development Guide

## Overview

Ring 0 is the kernel privilege level. At this level, you have:
- Full access to system memory (via `MmCopyVirtualMemory`)
- Direct access to kernel objects (DKOM)
- Ability to register/remove system callbacks
- Hardware access (I/O ports, MSRs)

## Prerequisites

### Software
```
✅ Windows 10/11 SDK
✅ Windows Driver Kit (WDK) 10/11
✅ Visual Studio 2022
✅ VMware Workstation / VirtualBox (for testing)
✅ WinDbg (kernel debugging)
```

### Knowledge
```
✅ C programming
✅ Windows kernel architecture
✅ Understanding of EPROCESS, ETHREAD structures
✅ Driver development basics (IRP, IOCTL)
```

## Setup Guide

### 1. Install WDK

```powershell
# Download from Microsoft
# https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

# After installation, WDK integrates with Visual Studio
```

### 2. Enable Test Signing

```cmd
# Run as Administrator
bcdedit /set testsigning on
# Reboot required
```

### 3. Setup Kernel Debugging (VM)

In VMware:
```
1. VM Settings → Add → Serial Port
2. Use named pipe: \\.\pipe\com_1
3. This end is the server
4. The other end is an application
```

Boot VM with debugging:
```cmd
bcdedit /debug on
bcdedit /dbgsettings serial debugport:1 baudrate:115200
```

Connect WinDbg:
```
File → Kernel Debug → COM → Port: \\.\pipe\com_1
```

## Key Concepts

### MmCopyVirtualMemory

The core function for cross-process memory access:

```c
NTSTATUS MmCopyVirtualMemory(
    PEPROCESS SourceProcess,      // Process to read FROM
    PVOID SourceAddress,          // Address in source
    PEPROCESS TargetProcess,      // Process to write TO
    PVOID TargetAddress,          // Address in target
    SIZE_T BufferSize,            // Size to copy
    KPROCESSOR_MODE PreviousMode, // KernelMode
    PSIZE_T ReturnSize            // Bytes copied
);
```

Example read:
```c
NTSTATUS ReadProcessMemory(ULONG Pid, PVOID Addr, PVOID Buffer, SIZE_T Size) {
    PEPROCESS process;
    PsLookupProcessByProcessId((HANDLE)Pid, &process);
    
    SIZE_T bytes;
    NTSTATUS status = MmCopyVirtualMemory(
        process,              // Source (target game)
        Addr,                 // Address to read
        PsGetCurrentProcess(), // Destination (our driver)
        Buffer,               // Our buffer
        Size,
        KernelMode,
        &bytes
    );
    
    ObDereferenceObject(process);
    return status;
}
```

### DKOM (Direct Kernel Object Manipulation)

Hide process by unlinking from `ActiveProcessLinks`:

```c
NTSTATUS HideProcess(ULONG Pid) {
    PEPROCESS process;
    PsLookupProcessByProcessId((HANDLE)Pid, &process);
    
    // Get ActiveProcessLinks (offset varies by Windows version!)
    PLIST_ENTRY links = (PLIST_ENTRY)((PUCHAR)process + 0x448);
    
    // Unlink
    links->Flink->Blink = links->Blink;
    links->Blink->Flink = links->Flink;
    
    // Self-reference to prevent crashes
    links->Flink = links;
    links->Blink = links;
    
    ObDereferenceObject(process);
    return STATUS_SUCCESS;
}
```

⚠️ **Warning**: DKOM triggers PatchGuard on modern Windows!

### Callback Manipulation

Anti-cheats register callbacks:
- `ObRegisterCallbacks` - Handle creation monitoring
- `PsSetCreateProcessNotifyRoutine` - Process creation
- `PsSetLoadImageNotifyRoutine` - DLL loading
- `CmRegisterCallback` - Registry access

To bypass:
1. Find callback array in ntoskrnl.exe
2. Replace callback function with stub
3. Or remove registration entirely

### IOCTL Communication

Usermode communicates with driver via IOCTL:

```c
// In driver
switch (IoControlCode) {
    case IOCTL_READ_MEMORY:
        PKB_READ_REQUEST req = (PKB_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
        status = KbReadProcessMemory(req->ProcessId, ...);
        break;
}

// In usermode
DeviceIoControl(hDriver, IOCTL_READ_MEMORY, &request, sizeof(request), ...);
```

## Driver Loading Methods

### 1. Service Control Manager (Normal)
```cpp
sc create MyDriver binPath= "C:\driver.sys" type= kernel
sc start MyDriver
```

### 2. Manual Mapping (Stealthy)
- Map driver sections manually
- Resolve imports
- Call DriverEntry
- No SCM registration

### 3. Vulnerable Driver Exploit
- Load signed vulnerable driver (e.g., Intel, Capcom)
- Exploit to map unsigned code
- Examples: kdmapper, drvmap

## EPROCESS Offsets

Offsets change between Windows versions!

| Field | Win10 22H2 | Win11 22H2 |
|-------|------------|------------|
| ActiveProcessLinks | 0x448 | 0x448 |
| ImageFileName | 0x5A8 | 0x5A8 |
| UniqueProcessId | 0x440 | 0x440 |
| DirectoryTableBase | 0x028 | 0x028 |
| Peb | 0x550 | 0x550 |

Use dynamic offset finding in production!

## PatchGuard Considerations

PatchGuard (Kernel Patch Protection) detects:
- Modification of system structures
- Hooking of kernel functions
- DKOM operations

**Bypass strategies:**
1. Hypervisor (EPT hooking) - Ring -1
2. Disable PatchGuard at boot (bootkit)
3. Time-of-check-time-of-use (unreliable)

## Building the Driver

### Visual Studio Method
1. File → New → Project → Kernel Mode Driver (KMDF)
2. Copy source files to project
3. Build → Build Solution

### Command Line
```cmd
msbuild driver.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Testing Checklist

- [ ] Test in VM first!
- [ ] Enable kernel debugging
- [ ] Have system restore point
- [ ] Test each feature individually
- [ ] Monitor for BSODs with !analyze -v

## Resources

- Windows Internals Book (Russinovich)
- [OSR Online](https://www.osronline.com/)
- [ReactOS Source](https://reactos.org/) - Open Windows clone
- Intel SDM Volume 3 (System Programming)

