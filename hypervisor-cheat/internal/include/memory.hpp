/**
 * HYPERVISOR CHEAT - Memory Utilities
 * Internal memory read/write for injected DLL
 */

#pragma once

#include <Windows.h>
#include <Psapi.h>
#include <cstdint>
#include <vector>
#include <string>

#pragma comment(lib, "psapi.lib")

namespace memory {

// ============================================
// Module Base
// ============================================

inline uintptr_t GetModuleBase(const char* moduleName) {
    static HMODULE cachedModule = nullptr;
    static std::string cachedName;
    
    if (cachedName == moduleName && cachedModule) {
        return reinterpret_cast<uintptr_t>(cachedModule);
    }
    
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (hModule) {
        cachedModule = hModule;
        cachedName = moduleName;
    }
    return reinterpret_cast<uintptr_t>(hModule);
}

inline size_t GetModuleSize(HMODULE hModule) {
    MODULEINFO info = {};
    if (GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info))) {
        return info.SizeOfImage;
    }
    return 0;
}

// ============================================
// Safe Memory Read
// ============================================

inline bool IsValidPtr(uintptr_t address) {
    if (address < 0x10000 || address > 0x7FFFFFFFFFFF) {
        return false;
    }
    return true;
}

template <typename T>
inline T Read(uintptr_t address) {
    if (!IsValidPtr(address)) {
        return T{};
    }
    
    __try {
        return *reinterpret_cast<T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

template <typename T>
inline bool Write(uintptr_t address, const T& value) {
    if (!IsValidPtr(address)) {
        return false;
    }
    
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool ReadBuffer(uintptr_t address, void* buffer, size_t size) {
    if (!IsValidPtr(address) || !buffer || size == 0) {
        return false;
    }
    
    __try {
        memcpy(buffer, reinterpret_cast<void*>(address), size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ============================================
// Pattern Scanning
// ============================================

inline uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask) {
    size_t patternLen = strlen(mask);
    
    for (size_t i = 0; i <= size - patternLen; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] == 'x' && 
                *reinterpret_cast<uint8_t*>(start + i + j) != static_cast<uint8_t>(pattern[j])) {
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

inline uintptr_t FindPattern(const char* moduleName, const char* pattern, const char* mask) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return 0;
    
    uintptr_t base = reinterpret_cast<uintptr_t>(hModule);
    size_t size = GetModuleSize(hModule);
    
    return FindPattern(base, size, pattern, mask);
}

// ============================================
// IDA-style Pattern Scanning
// ============================================

inline std::vector<uint8_t> ParseIDAPattern(const std::string& pattern) {
    std::vector<uint8_t> bytes;
    
    size_t i = 0;
    while (i < pattern.size()) {
        if (pattern[i] == ' ') {
            ++i;
            continue;
        }
        
        if (pattern[i] == '?') {
            bytes.push_back(0);
            while (i < pattern.size() && pattern[i] == '?') ++i;
        } else {
            char byte[3] = {pattern[i], pattern[i + 1], 0};
            bytes.push_back(static_cast<uint8_t>(strtol(byte, nullptr, 16)));
            i += 2;
        }
    }
    
    return bytes;
}

inline uintptr_t FindPatternIDA(const char* moduleName, const std::string& pattern) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return 0;
    
    uintptr_t base = reinterpret_cast<uintptr_t>(hModule);
    size_t size = GetModuleSize(hModule);
    
    auto bytes = ParseIDAPattern(pattern);
    std::string patternRaw(bytes.begin(), bytes.end());
    
    std::string mask;
    for (char c : pattern) {
        if (c == '?') mask += '?';
        else if (c != ' ' && mask.size() < bytes.size()) {
            // Only add 'x' for actual bytes
        }
    }
    
    // Rebuild mask properly
    mask.clear();
    size_t pos = 0;
    for (size_t i = 0; i < pattern.size() && pos < bytes.size(); ++i) {
        if (pattern[i] == ' ') continue;
        
        if (pattern[i] == '?') {
            mask += '?';
            while (i < pattern.size() && pattern[i] == '?') ++i;
            --i;
            ++pos;
        } else {
            mask += 'x';
            ++i; // Skip second hex char
            ++pos;
        }
    }
    
    return FindPattern(base, size, patternRaw.c_str(), mask.c_str());
}

} // namespace memory
