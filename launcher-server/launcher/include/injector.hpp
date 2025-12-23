#pragma once

/**
 * Cheat Launcher - DLL Injector
 * Various injection methods
 */

#include <string>
#include <cstdint>

namespace launcher {

// ============================================
// Injection Methods
// ============================================

enum class InjectionMethod {
    LoadLibrary,        // Classic LoadLibrary injection
    ManualMap,          // Manual PE mapping (more stealthy)
    ThreadHijack,       // Hijack existing thread
    QueueUserAPC,       // APC injection
    NtCreateThreadEx    // Using ntdll directly
};

// ============================================
// Injector Result
// ============================================

struct InjectionResult {
    bool success;
    std::string error;
    uintptr_t moduleBase;
};

// ============================================
// Injector Class
// ============================================

class Injector {
public:
    // Inject DLL into process by name
    static InjectionResult Inject(
        const std::string& processName,
        const std::string& dllPath,
        InjectionMethod method = InjectionMethod::LoadLibrary
    );
    
    // Inject DLL into process by PID
    static InjectionResult InjectByPid(
        uint32_t pid,
        const std::string& dllPath,
        InjectionMethod method = InjectionMethod::LoadLibrary
    );
    
    // Wait for process to start, then inject
    static InjectionResult WaitAndInject(
        const std::string& processName,
        const std::string& dllPath,
        InjectionMethod method = InjectionMethod::LoadLibrary,
        uint32_t timeoutMs = 30000
    );
    
    // Check if DLL is already injected
    static bool IsInjected(const std::string& processName, const std::string& dllName);
    
    // Eject DLL from process
    static bool Eject(const std::string& processName, const std::string& dllName);

private:
    static InjectionResult InjectLoadLibrary(uint32_t pid, const std::string& dllPath);
    static InjectionResult InjectManualMap(uint32_t pid, const std::string& dllPath);
    static InjectionResult InjectThreadHijack(uint32_t pid, const std::string& dllPath);
    static InjectionResult InjectAPC(uint32_t pid, const std::string& dllPath);
    static InjectionResult InjectNtCreateThread(uint32_t pid, const std::string& dllPath);
    
    static uint32_t FindProcessId(const std::string& processName);
};

} // namespace launcher

