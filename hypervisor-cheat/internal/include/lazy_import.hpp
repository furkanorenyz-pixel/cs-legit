/**
 * HYPERVISOR CHEAT - Lazy Import
 * Runtime API resolution for anti-detection
 * 
 * Usage: LI_FN(MessageBoxA)(nullptr, "text", "title", MB_OK);
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <intrin.h>

namespace lazy_detail {

// Compile-time hash
constexpr uint32_t Hash(const char* str) {
    uint32_t hash = 0x811c9dc5;
    while (*str) {
        hash ^= static_cast<uint8_t>(*str++);
        hash *= 0x01000193;
    }
    return hash;
}

// PEB walking
inline uintptr_t GetModuleBase(uint32_t hash) {
    // Get PEB
#ifdef _WIN64
    auto peb = reinterpret_cast<PEB*>(__readgsqword(0x60));
#else
    auto peb = reinterpret_cast<PEB*>(__readfsdword(0x30));
#endif
    
    auto ldr = peb->Ldr;
    auto list = &ldr->InMemoryOrderModuleList;
    
    for (auto entry = list->Flink; entry != list; entry = entry->Flink) {
        auto mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        
        if (!mod->BaseDllName.Buffer) continue;
        
        // Hash module name (lowercase)
        char name[256] = {};
        for (int i = 0; i < mod->BaseDllName.Length / 2 && i < 255; ++i) {
            char c = static_cast<char>(mod->BaseDllName.Buffer[i]);
            name[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        }
        
        if (Hash(name) == hash) {
            return reinterpret_cast<uintptr_t>(mod->DllBase);
        }
    }
    
    return 0;
}

inline uintptr_t GetExport(uintptr_t moduleBase, uint32_t hash) {
    if (!moduleBase) return 0;
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(moduleBase);
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(moduleBase + dosHeader->e_lfanew);
    
    auto exportDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!exportDir.VirtualAddress) return 0;
    
    auto exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(moduleBase + exportDir.VirtualAddress);
    auto names = reinterpret_cast<uint32_t*>(moduleBase + exports->AddressOfNames);
    auto ordinals = reinterpret_cast<uint16_t*>(moduleBase + exports->AddressOfNameOrdinals);
    auto functions = reinterpret_cast<uint32_t*>(moduleBase + exports->AddressOfFunctions);
    
    for (uint32_t i = 0; i < exports->NumberOfNames; ++i) {
        auto name = reinterpret_cast<const char*>(moduleBase + names[i]);
        
        if (Hash(name) == hash) {
            return moduleBase + functions[ordinals[i]];
        }
    }
    
    return 0;
}

template<typename T>
struct LazyFunction {
    uint32_t moduleHash;
    uint32_t functionHash;
    mutable T* cached = nullptr;
    
    constexpr LazyFunction(uint32_t mod, uint32_t func) 
        : moduleHash(mod), functionHash(func) {}
    
    T* get() const {
        if (!cached) {
            auto base = GetModuleBase(moduleHash);
            cached = reinterpret_cast<T*>(GetExport(base, functionHash));
        }
        return cached;
    }
    
    template<typename... Args>
    auto operator()(Args&&... args) const {
        return get()(std::forward<Args>(args)...);
    }
};

} // namespace lazy_detail

// Helper macros
#define LI_HASH(str) ::lazy_detail::Hash(str)

#define LI_FN(func) \
    ([]() { \
        static ::lazy_detail::LazyFunction<decltype(func)> lazy( \
            LI_HASH("kernel32.dll"), LI_HASH(#func)); \
        return lazy; \
    }())

#define LI_FN_MOD(mod, func) \
    ([]() { \
        static ::lazy_detail::LazyFunction<decltype(func)> lazy( \
            LI_HASH(mod), LI_HASH(#func)); \
        return lazy; \
    }())

