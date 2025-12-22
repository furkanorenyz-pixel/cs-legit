/**
 * HYPERVISOR CHEAT - Usermode Interface
 * Communication with Ring -1 hypervisor via VMCALL
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include "../../common/types.h"
#include "../../common/vmcall_codes.h"

namespace hv {

// ============================================
// Hypervisor Interface
// ============================================

class Hypervisor {
public:
    static Hypervisor& Get() {
        static Hypervisor instance;
        return instance;
    }
    
    // Initialization
    bool Initialize();
    void Shutdown();
    bool IsActive() const { return m_active; }
    
    // Info
    HV_INFO GetInfo() const { return m_info; }
    
    // Process operations
    std::optional<uint64_t> GetProcessCr3(uint32_t pid);
    std::optional<uint64_t> GetModuleBase(uint32_t pid, const char* moduleName);
    uint32_t FindProcess(const char* processName);
    
    // Memory operations
    template<typename T>
    T Read(uint64_t cr3, uint64_t address) {
        T value{};
        ReadMemory(cr3, address, &value, sizeof(T));
        return value;
    }
    
    template<typename T>
    bool Write(uint64_t cr3, uint64_t address, const T& value) {
        return WriteMemory(cr3, address, &value, sizeof(T));
    }
    
    bool ReadMemory(uint64_t cr3, uint64_t address, void* buffer, size_t size);
    bool WriteMemory(uint64_t cr3, uint64_t address, const void* buffer, size_t size);
    
    // Game-specific (optimized batch read)
    bool ReadGameData(uint32_t pid, GAME_DATA* data);
    
private:
    Hypervisor() = default;
    ~Hypervisor() { Shutdown(); }
    
    // Prevent copying
    Hypervisor(const Hypervisor&) = delete;
    Hypervisor& operator=(const Hypervisor&) = delete;
    
    // VMCALL wrapper
    uint64_t Vmcall(uint64_t number, uint64_t p1, uint64_t p2, uint64_t p3);
    
    bool m_active = false;
    HV_INFO m_info{};
};

// ============================================
// Convenience Functions
// ============================================

inline bool Init() {
    return Hypervisor::Get().Initialize();
}

inline bool IsActive() {
    return Hypervisor::Get().IsActive();
}

template<typename T>
inline T Read(uint64_t cr3, uint64_t address) {
    return Hypervisor::Get().Read<T>(cr3, address);
}

template<typename T>
inline bool Write(uint64_t cr3, uint64_t address, const T& value) {
    return Hypervisor::Get().Write<T>(cr3, address, value);
}

} // namespace hv

