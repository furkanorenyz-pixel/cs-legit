/**
 * HYPERVISOR CHEAT - Internal Memory Utilities
 * Pattern scanning, module helpers
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>

namespace mem {

// ============================================
// Module Info
// ============================================

struct ModuleInfo {
    uintptr_t base;
    size_t size;
    std::string name;
};

inline ModuleInfo GetModule(const char* moduleName) {
    ModuleInfo info = {};
    
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) return info;
    
    MODULEINFO modInfo;
    if (GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo))) {
        info.base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        info.size = modInfo.SizeOfImage;
        info.name = moduleName;
    }
    
    return info;
}

// ============================================
// Pattern Scanning
// ============================================

inline std::vector<int> PatternToBytes(const char* pattern) {
    std::vector<int> bytes;
    const char* start = pattern;
    const char* end = pattern + strlen(pattern);
    
    for (const char* current = start; current < end; ++current) {
        if (*current == '?') {
            ++current;
            if (*current == '?') ++current;
            bytes.push_back(-1);
        } else {
            bytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
        }
    }
    
    return bytes;
}

inline uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern) {
    auto bytes = PatternToBytes(pattern);
    auto scanBytes = reinterpret_cast<uint8_t*>(start);
    auto patternSize = bytes.size();
    auto patternData = bytes.data();
    
    for (size_t i = 0; i < size - patternSize; ++i) {
        bool found = true;
        
        for (size_t j = 0; j < patternSize; ++j) {
            if (patternData[j] != -1 && scanBytes[i + j] != patternData[j]) {
                found = false;
                break;
            }
        }
        
        if (found) {
            return start + i;
        }
    }
    
    return 0;
}

inline uintptr_t FindPattern(const char* moduleName, const char* pattern) {
    auto mod = GetModule(moduleName);
    if (!mod.base) return 0;
    return FindPattern(mod.base, mod.size, pattern);
}

// ============================================
// Relative Address Resolution
// ============================================

inline uintptr_t GetAbsoluteAddress(uintptr_t instruction, int offset, int size) {
    if (!instruction) return 0;
    int relative = *reinterpret_cast<int*>(instruction + offset);
    return instruction + size + relative;
}

// ============================================
// Safe Memory Operations
// ============================================

template<typename T>
inline T Read(uintptr_t address) {
    if (!address) return T{};
    __try {
        return *reinterpret_cast<T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

template<typename T>
inline bool Write(uintptr_t address, const T& value) {
    if (!address) return false;
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool ReadBuffer(uintptr_t address, void* buffer, size_t size) {
    if (!address || !buffer) return false;
    __try {
        memcpy(buffer, reinterpret_cast<void*>(address), size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ============================================
// Virtual Protect Helper
// ============================================

class ScopedVirtualProtect {
public:
    ScopedVirtualProtect(void* address, size_t size, DWORD newProtect)
        : m_address(address), m_size(size), m_oldProtect(0) {
        VirtualProtect(m_address, m_size, newProtect, &m_oldProtect);
    }
    
    ~ScopedVirtualProtect() {
        DWORD temp;
        VirtualProtect(m_address, m_size, m_oldProtect, &temp);
    }
    
private:
    void* m_address;
    size_t m_size;
    DWORD m_oldProtect;
};

} // namespace mem

