/*
 * KERNEL BYPASS FRAMEWORK
 * Unified Usermode Interface
 * 
 * Single API for all ring levels:
 * - Ring 3: Direct syscall / ReadProcessMemory
 * - Ring 0: Kernel driver
 * - Ring -1: Hypervisor
 * - Ring -2/-3: SMM/Firmware
 * 
 * Auto-detects best available backend
 */

#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

#include "../../common/types.h"
#include "../../common/ioctl_codes.h"

namespace kb {

// ============================================
// Forward Declarations
// ============================================

class KernelInterface;
class Backend;

// ============================================
// Process Information
// ============================================

struct ProcessInfo {
    DWORD pid;
    std::string name;
    ULONG64 baseAddress;
    ULONG64 cr3;
    bool isWow64;
    bool isProtected;
};

struct ModuleInfo {
    std::string name;
    std::string path;
    ULONG64 base;
    ULONG64 size;
};

// ============================================
// Backend Interface (Abstract)
// ============================================

class Backend {
public:
    virtual ~Backend() = default;
    
    // Initialization
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsAvailable() const = 0;
    virtual RING_LEVEL GetRingLevel() const = 0;
    virtual const char* GetName() const = 0;
    
    // Memory Operations
    virtual KB_STATUS ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) = 0;
    virtual KB_STATUS WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) = 0;
    
    // Process Operations
    virtual KB_STATUS GetProcessInfo(DWORD pid, ProcessInfo& info) = 0;
    virtual KB_STATUS GetProcessBase(DWORD pid, ULONG64& base) = 0;
    virtual KB_STATUS GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) = 0;
    
    // HWID Operations
    virtual KB_STATUS SpoofHwid(HWID_TYPE type, const char* value) = 0;
    virtual KB_STATUS RandomizeHwid() = 0;
    
    // Advanced (may not be supported by all backends)
    virtual KB_STATUS HideProcess(DWORD pid) { return KB_ERROR_NOT_INITIALIZED; }
    virtual KB_STATUS ProtectProcess(DWORD pid) { return KB_ERROR_NOT_INITIALIZED; }
    virtual KB_STATUS DisableCallbacks() { return KB_ERROR_NOT_INITIALIZED; }
};

// ============================================
// Backend: Usermode (Ring 3)
// ============================================

class UsermodeBackend : public Backend {
public:
    bool Initialize() override;
    void Shutdown() override;
    bool IsAvailable() const override { return m_available; }
    RING_LEVEL GetRingLevel() const override { return RING_LEVEL_USER; }
    const char* GetName() const override { return "Usermode (Ring 3)"; }
    
    KB_STATUS ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) override;
    KB_STATUS WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) override;
    KB_STATUS GetProcessInfo(DWORD pid, ProcessInfo& info) override;
    KB_STATUS GetProcessBase(DWORD pid, ULONG64& base) override;
    KB_STATUS GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) override;
    KB_STATUS SpoofHwid(HWID_TYPE type, const char* value) override;
    KB_STATUS RandomizeHwid() override;

private:
    bool m_available = false;
    HANDLE m_processHandle = nullptr;
    DWORD m_currentPid = 0;
};

// ============================================
// Backend: Kernel Driver (Ring 0)
// ============================================

class KernelDriverBackend : public Backend {
public:
    bool Initialize() override;
    void Shutdown() override;
    bool IsAvailable() const override { return m_driverHandle != INVALID_HANDLE_VALUE; }
    RING_LEVEL GetRingLevel() const override { return RING_LEVEL_KERNEL; }
    const char* GetName() const override { return "Kernel Driver (Ring 0)"; }
    
    KB_STATUS ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) override;
    KB_STATUS WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) override;
    KB_STATUS GetProcessInfo(DWORD pid, ProcessInfo& info) override;
    KB_STATUS GetProcessBase(DWORD pid, ULONG64& base) override;
    KB_STATUS GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) override;
    KB_STATUS SpoofHwid(HWID_TYPE type, const char* value) override;
    KB_STATUS RandomizeHwid() override;
    
    // Advanced Ring 0 operations
    KB_STATUS HideProcess(DWORD pid) override;
    KB_STATUS ProtectProcess(DWORD pid) override;
    KB_STATUS DisableCallbacks() override;

private:
    HANDLE m_driverHandle = INVALID_HANDLE_VALUE;
    
    bool SendIoctl(DWORD code, void* inBuffer, DWORD inSize, void* outBuffer, DWORD outSize);
};

// ============================================
// Backend: Hypervisor (Ring -1)
// ============================================

class HypervisorBackend : public Backend {
public:
    bool Initialize() override;
    void Shutdown() override;
    bool IsAvailable() const override { return m_active; }
    RING_LEVEL GetRingLevel() const override { return RING_LEVEL_HYPER; }
    const char* GetName() const override { return "Hypervisor (Ring -1)"; }
    
    KB_STATUS ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) override;
    KB_STATUS WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) override;
    KB_STATUS GetProcessInfo(DWORD pid, ProcessInfo& info) override;
    KB_STATUS GetProcessBase(DWORD pid, ULONG64& base) override;
    KB_STATUS GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) override;
    KB_STATUS SpoofHwid(HWID_TYPE type, const char* value) override;
    KB_STATUS RandomizeHwid() override;
    
    // Hypervisor-specific
    KB_STATUS SpoofCpuid(DWORD function, DWORD eax, DWORD ebx, DWORD ecx, DWORD edx);
    KB_STATUS EptHook(ULONG64 target, ULONG64 hook);
    KB_STATUS EptUnhook(ULONG64 target);

private:
    bool m_active = false;
    HANDLE m_driverHandle = INVALID_HANDLE_VALUE;
    
    ULONG64 Vmcall(ULONG64 number, ULONG64 p1, ULONG64 p2, ULONG64 p3);
};

// ============================================
// Backend: Firmware/SMM (Ring -2/-3)
// ============================================

class FirmwareBackend : public Backend {
public:
    bool Initialize() override;
    void Shutdown() override;
    bool IsAvailable() const override { return m_available; }
    RING_LEVEL GetRingLevel() const override { return RING_LEVEL_FIRMWARE; }
    const char* GetName() const override { return "Firmware (Ring -3)"; }
    
    KB_STATUS ReadMemory(DWORD pid, ULONG64 address, void* buffer, size_t size) override;
    KB_STATUS WriteMemory(DWORD pid, ULONG64 address, const void* buffer, size_t size) override;
    KB_STATUS GetProcessInfo(DWORD pid, ProcessInfo& info) override;
    KB_STATUS GetProcessBase(DWORD pid, ULONG64& base) override;
    KB_STATUS GetModuleBase(DWORD pid, const char* moduleName, ULONG64& base) override;
    KB_STATUS SpoofHwid(HWID_TYPE type, const char* value) override;
    KB_STATUS RandomizeHwid() override;
    
    // Firmware-specific
    KB_STATUS ReadPhysical(ULONG64 physAddr, void* buffer, size_t size);
    KB_STATUS WritePhysical(ULONG64 physAddr, const void* buffer, size_t size);

private:
    bool m_available = false;
    
    // Communication with SMM handler
    bool TriggerSmi(void* commBuffer);
};

// ============================================
// Main Interface Class
// ============================================

class KernelInterface {
public:
    KernelInterface() = default;
    ~KernelInterface();
    
    // Initialization
    bool Initialize(BACKEND_TYPE preferredBackend = BACKEND_AUTO);
    void Shutdown();
    
    // Status
    bool IsInitialized() const { return m_backend != nullptr && m_backend->IsAvailable(); }
    RING_LEVEL GetRingLevel() const { return m_backend ? m_backend->GetRingLevel() : RING_LEVEL_USER; }
    const char* GetBackendName() const { return m_backend ? m_backend->GetName() : "None"; }
    
    // ====== Memory Operations ======
    
    template<typename T>
    T Read(DWORD pid, ULONG64 address) {
        T value{};
        if (m_backend) {
            m_backend->ReadMemory(pid, address, &value, sizeof(T));
        }
        return value;
    }
    
    template<typename T>
    bool Write(DWORD pid, ULONG64 address, const T& value) {
        if (!m_backend) return false;
        return m_backend->WriteMemory(pid, address, &value, sizeof(T)) == KB_SUCCESS;
    }
    
    bool ReadBuffer(DWORD pid, ULONG64 address, void* buffer, size_t size) {
        if (!m_backend) return false;
        return m_backend->ReadMemory(pid, address, buffer, size) == KB_SUCCESS;
    }
    
    bool WriteBuffer(DWORD pid, ULONG64 address, const void* buffer, size_t size) {
        if (!m_backend) return false;
        return m_backend->WriteMemory(pid, address, buffer, size) == KB_SUCCESS;
    }
    
    // ====== Process Operations ======
    
    std::optional<ProcessInfo> GetProcessInfo(DWORD pid);
    std::optional<ULONG64> GetProcessBase(DWORD pid);
    std::optional<ULONG64> GetModuleBase(DWORD pid, const char* moduleName);
    std::vector<ModuleInfo> GetModules(DWORD pid);
    
    // Process by name
    DWORD FindProcess(const char* processName);
    
    // ====== HWID Operations ======
    
    bool SpoofDiskSerial(const char* serial);
    bool SpoofMacAddress(const BYTE mac[6]);
    bool SpoofSmbios(const char* serial);
    bool RandomizeAllHwid();
    
    // ====== Advanced Operations ======
    
    bool HideProcess(DWORD pid);
    bool ProtectProcess(DWORD pid);
    bool DisableAntiCheatCallbacks();
    
    // Hypervisor-specific (if available)
    bool SpoofCpuid(DWORD function, DWORD eax, DWORD ebx, DWORD ecx, DWORD edx);
    bool EptHook(ULONG64 target, ULONG64 hook);
    
private:
    std::unique_ptr<Backend> m_backend;
    
    std::unique_ptr<Backend> CreateBackend(BACKEND_TYPE type);
    bool TryInitializeBackend(BACKEND_TYPE type);
};

// ============================================
// Global Instance (optional convenience)
// ============================================

// Singleton accessor
KernelInterface& GetKernelInterface();

// Quick initialization
inline bool InitializeKernel(BACKEND_TYPE backend = BACKEND_AUTO) {
    return GetKernelInterface().Initialize(backend);
}

// ============================================
// Helper Macros
// ============================================

#define KB_READ(pid, addr, type)        GetKernelInterface().Read<type>(pid, addr)
#define KB_WRITE(pid, addr, val)        GetKernelInterface().Write(pid, addr, val)
#define KB_PROCESS_BASE(pid)            GetKernelInterface().GetProcessBase(pid)
#define KB_MODULE_BASE(pid, name)       GetKernelInterface().GetModuleBase(pid, name)

} // namespace kb

