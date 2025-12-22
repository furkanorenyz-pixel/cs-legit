/*
 * KERNEL BYPASS FRAMEWORK
 * Unified Interface Implementation
 */

#include "../include/kernel_interface.hpp"
#include <TlHelp32.h>
#include <algorithm>

namespace kb {

// ============================================
// Global Instance
// ============================================

static std::unique_ptr<KernelInterface> g_instance;

KernelInterface& GetKernelInterface() {
    if (!g_instance) {
        g_instance = std::make_unique<KernelInterface>();
    }
    return *g_instance;
}

// ============================================
// KernelInterface Implementation
// ============================================

KernelInterface::~KernelInterface() {
    Shutdown();
}

bool KernelInterface::Initialize(BACKEND_TYPE preferredBackend) {
    if (m_backend && m_backend->IsAvailable()) {
        return true;  // Already initialized
    }
    
    if (preferredBackend != BACKEND_AUTO) {
        // Try specific backend
        return TryInitializeBackend(preferredBackend);
    }
    
    // Auto-detect: Try from highest to lowest privilege
    // Ring -3 -> Ring -1 -> Ring 0 -> Ring 3
    
    static const BACKEND_TYPE backends[] = {
        BACKEND_FIRMWARE,
        BACKEND_HYPERVISOR,
        BACKEND_DRIVER,
        BACKEND_SYSCALL,
        BACKEND_USERMODE
    };
    
    for (auto type : backends) {
        if (TryInitializeBackend(type)) {
            return true;
        }
    }
    
    return false;
}

bool KernelInterface::TryInitializeBackend(BACKEND_TYPE type) {
    auto backend = CreateBackend(type);
    if (!backend) {
        return false;
    }
    
    if (backend->Initialize()) {
        m_backend = std::move(backend);
        return true;
    }
    
    return false;
}

std::unique_ptr<Backend> KernelInterface::CreateBackend(BACKEND_TYPE type) {
    switch (type) {
        case BACKEND_USERMODE:
        case BACKEND_SYSCALL:
            return std::make_unique<UsermodeBackend>();
            
        case BACKEND_DRIVER:
        case BACKEND_VULNERABLE_DRIVER:
            return std::make_unique<KernelDriverBackend>();
            
        case BACKEND_HYPERVISOR:
            return std::make_unique<HypervisorBackend>();
            
        case BACKEND_SMM:
        case BACKEND_FIRMWARE:
            return std::make_unique<FirmwareBackend>();
            
        default:
            return nullptr;
    }
}

void KernelInterface::Shutdown() {
    if (m_backend) {
        m_backend->Shutdown();
        m_backend.reset();
    }
}

std::optional<ProcessInfo> KernelInterface::GetProcessInfo(DWORD pid) {
    if (!m_backend) return std::nullopt;
    
    ProcessInfo info;
    if (m_backend->GetProcessInfo(pid, info) == KB_SUCCESS) {
        return info;
    }
    return std::nullopt;
}

std::optional<ULONG64> KernelInterface::GetProcessBase(DWORD pid) {
    if (!m_backend) return std::nullopt;
    
    ULONG64 base = 0;
    if (m_backend->GetProcessBase(pid, base) == KB_SUCCESS) {
        return base;
    }
    return std::nullopt;
}

std::optional<ULONG64> KernelInterface::GetModuleBase(DWORD pid, const char* moduleName) {
    if (!m_backend || !moduleName) return std::nullopt;
    
    ULONG64 base = 0;
    if (m_backend->GetModuleBase(pid, moduleName, base) == KB_SUCCESS) {
        return base;
    }
    return std::nullopt;
}

DWORD KernelInterface::FindProcess(const char* processName) {
    if (!processName) return 0;
    
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry{};
        entry.dwSize = sizeof(entry);
        
        if (Process32First(snap, &entry)) {
            do {
                if (_stricmp(entry.szExeFile, processName) == 0) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snap, &entry));
        }
        
        CloseHandle(snap);
    }
    
    return pid;
}

std::vector<ModuleInfo> KernelInterface::GetModules(DWORD pid) {
    std::vector<ModuleInfo> modules;
    
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 entry{};
        entry.dwSize = sizeof(entry);
        
        if (Module32First(snap, &entry)) {
            do {
                ModuleInfo info;
                info.name = entry.szModule;
                info.path = entry.szExePath;
                info.base = (ULONG64)entry.modBaseAddr;
                info.size = entry.modBaseSize;
                modules.push_back(info);
            } while (Module32Next(snap, &entry));
        }
        
        CloseHandle(snap);
    }
    
    return modules;
}

// HWID Operations
bool KernelInterface::SpoofDiskSerial(const char* serial) {
    if (!m_backend) return false;
    return m_backend->SpoofHwid(HWID_DISK_SERIAL, serial) == KB_SUCCESS;
}

bool KernelInterface::SpoofMacAddress(const BYTE mac[6]) {
    if (!m_backend) return false;
    char macStr[18];
    sprintf_s(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return m_backend->SpoofHwid(HWID_MAC_ADDRESS, macStr) == KB_SUCCESS;
}

bool KernelInterface::SpoofSmbios(const char* serial) {
    if (!m_backend) return false;
    return m_backend->SpoofHwid(HWID_SMBIOS, serial) == KB_SUCCESS;
}

bool KernelInterface::RandomizeAllHwid() {
    if (!m_backend) return false;
    return m_backend->RandomizeHwid() == KB_SUCCESS;
}

// Advanced Operations
bool KernelInterface::HideProcess(DWORD pid) {
    if (!m_backend) return false;
    return m_backend->HideProcess(pid) == KB_SUCCESS;
}

bool KernelInterface::ProtectProcess(DWORD pid) {
    if (!m_backend) return false;
    return m_backend->ProtectProcess(pid) == KB_SUCCESS;
}

bool KernelInterface::DisableAntiCheatCallbacks() {
    if (!m_backend) return false;
    return m_backend->DisableCallbacks() == KB_SUCCESS;
}

bool KernelInterface::SpoofCpuid(DWORD function, DWORD eax, DWORD ebx, DWORD ecx, DWORD edx) {
    auto* hyperBackend = dynamic_cast<HypervisorBackend*>(m_backend.get());
    if (!hyperBackend) return false;
    return hyperBackend->SpoofCpuid(function, eax, ebx, ecx, edx) == KB_SUCCESS;
}

bool KernelInterface::EptHook(ULONG64 target, ULONG64 hook) {
    auto* hyperBackend = dynamic_cast<HypervisorBackend*>(m_backend.get());
    if (!hyperBackend) return false;
    return hyperBackend->EptHook(target, hook) == KB_SUCCESS;
}

// ============================================
// UsermodeBackend Implementation
// ============================================

bool UsermodeBackend::Initialize() {
    m_available = true;
    return true;
}

void UsermodeBackend::Shutdown() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
    }
    m_available = false;
}

KB_STATUS UsermodeBackend::ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) {
    if (m_currentPid != pid || !m_processHandle) {
        if (m_processHandle) {
            CloseHandle(m_processHandle);
        }
        m_processHandle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        m_currentPid = pid;
    }
    
    if (!m_processHandle) {
        return KB_ERROR_ACCESS_DENIED;
    }
    
    SIZE_T bytesRead = 0;
    if (ReadProcessMemory(m_processHandle, (LPCVOID)address, buffer, size, &bytesRead)) {
        return KB_SUCCESS;
    }
    
    return KB_ERROR_READ_FAILED;
}

KB_STATUS UsermodeBackend::WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProcess) {
        return KB_ERROR_ACCESS_DENIED;
    }
    
    SIZE_T bytesWritten = 0;
    BOOL result = WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesWritten);
    
    CloseHandle(hProcess);
    
    return result ? KB_SUCCESS : KB_ERROR_WRITE_FAILED;
}

KB_STATUS UsermodeBackend::GetProcessInfo(DWORD pid, ProcessInfo& info) {
    info.pid = pid;
    
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return KB_ERROR_PROCESS_NOT_FOUND;
    }
    
    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    
    bool found = false;
    if (Process32First(snap, &entry)) {
        do {
            if (entry.th32ProcessID == pid) {
                info.name = entry.szExeFile;
                found = true;
                break;
            }
        } while (Process32Next(snap, &entry));
    }
    
    CloseHandle(snap);
    
    if (!found) {
        return KB_ERROR_PROCESS_NOT_FOUND;
    }
    
    // Get base address
    GetProcessBase(pid, info.baseAddress);
    
    // Check WoW64
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        BOOL wow64 = FALSE;
        IsWow64Process(hProcess, &wow64);
        info.isWow64 = wow64 != FALSE;
        CloseHandle(hProcess);
    }
    
    return KB_SUCCESS;
}

KB_STATUS UsermodeBackend::GetProcessBase(DWORD pid, ULONG64& base) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) {
        return KB_ERROR_MODULE_NOT_FOUND;
    }
    
    MODULEENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    
    if (Module32First(snap, &entry)) {
        base = (ULONG64)entry.modBaseAddr;
        CloseHandle(snap);
        return KB_SUCCESS;
    }
    
    CloseHandle(snap);
    return KB_ERROR_MODULE_NOT_FOUND;
}

KB_STATUS UsermodeBackend::GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) {
        return KB_ERROR_MODULE_NOT_FOUND;
    }
    
    MODULEENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    
    if (Module32First(snap, &entry)) {
        do {
            if (_stricmp(entry.szModule, moduleName) == 0) {
                base = (ULONG64)entry.modBaseAddr;
                CloseHandle(snap);
                return KB_SUCCESS;
            }
        } while (Module32Next(snap, &entry));
    }
    
    CloseHandle(snap);
    return KB_ERROR_MODULE_NOT_FOUND;
}

KB_STATUS UsermodeBackend::SpoofHwid(HWID_TYPE type, const char* value) {
    // Usermode can only spoof via registry
    (void)type;
    (void)value;
    return KB_ERROR_ACCESS_DENIED;  // Need kernel driver
}

KB_STATUS UsermodeBackend::RandomizeHwid() {
    return KB_ERROR_ACCESS_DENIED;  // Need kernel driver
}

// ============================================
// KernelDriverBackend Implementation
// ============================================

bool KernelDriverBackend::Initialize() {
    m_driverHandle = CreateFileW(
        L"\\\\.\\KernelBypass",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (m_driverHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Ping driver
    DWORD response = 0;
    if (SendIoctl(IOCTL_KB_PING, nullptr, 0, &response, sizeof(response))) {
        return response == 0xDEADBEEF;
    }
    
    CloseHandle(m_driverHandle);
    m_driverHandle = INVALID_HANDLE_VALUE;
    return false;
}

void KernelDriverBackend::Shutdown() {
    if (m_driverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_driverHandle);
        m_driverHandle = INVALID_HANDLE_VALUE;
    }
}

bool KernelDriverBackend::SendIoctl(DWORD code, void* inBuffer, DWORD inSize, void* outBuffer, DWORD outSize) {
    if (m_driverHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DWORD bytesReturned = 0;
    return DeviceIoControl(
        m_driverHandle,
        code,
        inBuffer, inSize,
        outBuffer, outSize,
        &bytesReturned,
        nullptr
    ) != FALSE;
}

KB_STATUS KernelDriverBackend::ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) {
    KB_READ_REQUEST request{};
    request.ProcessId = pid;
    request.Address = address;
    request.Buffer = (ULONG64)buffer;
    request.Size = size;
    
    if (SendIoctl(IOCTL_KB_READ_MEMORY, &request, sizeof(request), &request, sizeof(request))) {
        return request.Status;
    }
    
    return KB_ERROR_READ_FAILED;
}

KB_STATUS KernelDriverBackend::WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) {
    KB_WRITE_REQUEST request{};
    request.ProcessId = pid;
    request.Address = address;
    request.Buffer = (ULONG64)buffer;
    request.Size = size;
    
    if (SendIoctl(IOCTL_KB_WRITE_MEMORY, &request, sizeof(request), &request, sizeof(request))) {
        return request.Status;
    }
    
    return KB_ERROR_WRITE_FAILED;
}

KB_STATUS KernelDriverBackend::GetProcessInfo(DWORD pid, ProcessInfo& info) {
    KB_PROCESS_INFO kbInfo{};
    kbInfo.ProcessId = pid;
    
    if (SendIoctl(IOCTL_KB_GET_PROCESS, &kbInfo, sizeof(kbInfo), &kbInfo, sizeof(kbInfo))) {
        info.pid = kbInfo.ProcessId;
        info.baseAddress = kbInfo.BaseAddress;
        info.cr3 = kbInfo.Cr3;
        info.isWow64 = kbInfo.IsWow64 != 0;
        info.isProtected = kbInfo.IsProtected != 0;
        info.name = kbInfo.Name;
        return KB_SUCCESS;
    }
    
    return KB_ERROR_PROCESS_NOT_FOUND;
}

KB_STATUS KernelDriverBackend::GetProcessBase(DWORD pid, ULONG64& base) {
    if (SendIoctl(IOCTL_KB_GET_PROCESS_BASE, &pid, sizeof(pid), &base, sizeof(base))) {
        return KB_SUCCESS;
    }
    return KB_ERROR_PROCESS_NOT_FOUND;
}

KB_STATUS KernelDriverBackend::GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) {
    KB_MODULE_INFO info{};
    strncpy_s(info.Name, moduleName, sizeof(info.Name) - 1);
    
    // Would need to add process ID to request
    if (SendIoctl(IOCTL_KB_GET_MODULE, &info, sizeof(info), &info, sizeof(info))) {
        base = info.BaseAddress;
        return KB_SUCCESS;
    }
    
    // Fallback to usermode enumeration
    return UsermodeBackend().GetModuleBase(pid, moduleName, base);
}

KB_STATUS KernelDriverBackend::SpoofHwid(HWID_TYPE type, const char* value) {
    KB_HWID_SPOOF spoof{};
    spoof.Type = type;
    strncpy_s(spoof.Spoofed, value, sizeof(spoof.Spoofed) - 1);
    
    if (SendIoctl(IOCTL_KB_SPOOF_HWID, &spoof, sizeof(spoof), &spoof, sizeof(spoof))) {
        return KB_SUCCESS;
    }
    
    return KB_ERROR_HWID_SPOOF_FAILED;
}

KB_STATUS KernelDriverBackend::RandomizeHwid() {
    if (SendIoctl(IOCTL_KB_RANDOMIZE_HWID, nullptr, 0, nullptr, 0)) {
        return KB_SUCCESS;
    }
    return KB_ERROR_HWID_SPOOF_FAILED;
}

KB_STATUS KernelDriverBackend::HideProcess(DWORD pid) {
    if (SendIoctl(IOCTL_KB_HIDE_PROCESS, &pid, sizeof(pid), nullptr, 0)) {
        return KB_SUCCESS;
    }
    return KB_ERROR_GENERIC;
}

KB_STATUS KernelDriverBackend::ProtectProcess(DWORD pid) {
    if (SendIoctl(IOCTL_KB_PROTECT_PROCESS, &pid, sizeof(pid), nullptr, 0)) {
        return KB_SUCCESS;
    }
    return KB_ERROR_GENERIC;
}

KB_STATUS KernelDriverBackend::DisableCallbacks() {
    if (SendIoctl(IOCTL_KB_DISABLE_ALL, nullptr, 0, nullptr, 0)) {
        return KB_SUCCESS;
    }
    return KB_ERROR_GENERIC;
}

// ============================================
// HypervisorBackend Implementation (Stub)
// ============================================

bool HypervisorBackend::Initialize() {
    // Try to open hypervisor driver
    m_driverHandle = CreateFileW(
        L"\\\\.\\KernelBypassHV",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (m_driverHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Check if hypervisor is active via VMCALL
    ULONG64 result = Vmcall(VMCALL_TEST, 0, 0, 0);
    m_active = (result == 0x1);
    
    if (!m_active) {
        CloseHandle(m_driverHandle);
        m_driverHandle = INVALID_HANDLE_VALUE;
    }
    
    return m_active;
}

void HypervisorBackend::Shutdown() {
    m_active = false;
    if (m_driverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_driverHandle);
        m_driverHandle = INVALID_HANDLE_VALUE;
    }
}

ULONG64 HypervisorBackend::Vmcall(ULONG64 number, ULONG64 p1, ULONG64 p2, ULONG64 p3) {
    KB_VMCALL_REQUEST request{};
    request.VmcallNumber = number;
    request.Param1 = p1;
    request.Param2 = p2;
    request.Param3 = p3;
    
    DWORD bytesReturned = 0;
    if (DeviceIoControl(m_driverHandle, IOCTL_KB_VMCALL, 
        &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr)) {
        return request.ReturnValue;
    }
    
    return 0;
}

KB_STATUS HypervisorBackend::ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) {
    // Use VMCALL to read memory
    // This would read via hypervisor's EPT
    (void)pid; (void)address; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) {
    (void)pid; (void)address; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::GetProcessInfo(DWORD pid, ProcessInfo& info) {
    (void)pid; (void)info;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::GetProcessBase(DWORD pid, ULONG64& base) {
    (void)pid; (void)base;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) {
    (void)pid; (void)moduleName; (void)base;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::SpoofHwid(HWID_TYPE type, const char* value) {
    (void)type; (void)value;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::RandomizeHwid() {
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::SpoofCpuid(DWORD function, DWORD eax, DWORD ebx, DWORD ecx, DWORD edx) {
    // Would use VMCALL to register CPUID spoof
    (void)function; (void)eax; (void)ebx; (void)ecx; (void)edx;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::EptHook(ULONG64 target, ULONG64 hook) {
    (void)target; (void)hook;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

KB_STATUS HypervisorBackend::EptUnhook(ULONG64 target) {
    (void)target;
    return KB_ERROR_NOT_INITIALIZED;  // Stub
}

// ============================================
// FirmwareBackend Implementation (Stub)
// ============================================

bool FirmwareBackend::Initialize() {
    // Check if SMM handler is installed
    // This would require specific detection method
    m_available = false;  // Disabled by default - very dangerous!
    return m_available;
}

void FirmwareBackend::Shutdown() {
    m_available = false;
}

bool FirmwareBackend::TriggerSmi(void* commBuffer) {
    // Trigger SMI via I/O port (requires Ring 0)
    (void)commBuffer;
    return false;  // Stub
}

KB_STATUS FirmwareBackend::ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) {
    (void)pid; (void)address; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) {
    (void)pid; (void)address; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::GetProcessInfo(DWORD pid, ProcessInfo& info) {
    (void)pid; (void)info;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::GetProcessBase(DWORD pid, ULONG64& base) {
    (void)pid; (void)base;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) {
    (void)pid; (void)moduleName; (void)base;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::SpoofHwid(HWID_TYPE type, const char* value) {
    (void)type; (void)value;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::RandomizeHwid() {
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::ReadPhysical(ULONG64 physAddr, void* buffer, size_t size) {
    (void)physAddr; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;
}

KB_STATUS FirmwareBackend::WritePhysical(ULONG64 physAddr, const void* buffer, size_t size) {
    (void)physAddr; (void)buffer; (void)size;
    return KB_ERROR_NOT_INITIALIZED;
}

} // namespace kb

