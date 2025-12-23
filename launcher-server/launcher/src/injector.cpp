/**
 * Cheat Launcher - DLL Injector Implementation
 */

#include "../include/injector.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#include <fstream>
#include <thread>
#include <chrono>
#endif

namespace launcher {

#ifdef _WIN32

// ============================================
// Process Utilities
// ============================================

uint32_t Injector::FindProcessId(const std::string& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    
    uint32_t pid = 0;
    
    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, processName.c_str()) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return pid;
}

// ============================================
// Injection Methods
// ============================================

InjectionResult Injector::InjectLoadLibrary(uint32_t pid, const std::string& dllPath) {
    InjectionResult result = {false, "", 0};
    
    // Get absolute path
    char fullPath[MAX_PATH];
    GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullPath, nullptr);
    
    // Open process
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid
    );
    
    if (!hProcess) {
        result.error = "Failed to open process";
        return result;
    }
    
    // Allocate memory for DLL path
    size_t pathLen = strlen(fullPath) + 1;
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathLen,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remotePath) {
        CloseHandle(hProcess);
        result.error = "Failed to allocate memory";
        return result;
    }
    
    // Write DLL path
    if (!WriteProcessMemory(hProcess, remotePath, fullPath, pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        result.error = "Failed to write memory";
        return result;
    }
    
    // Get LoadLibraryA address
    LPVOID loadLibrary = (LPVOID)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    
    if (!loadLibrary) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        result.error = "Failed to get LoadLibraryA";
        return result;
    }
    
    // Create remote thread
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadLibrary, remotePath, 0, nullptr);
    
    if (!hThread) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        result.error = "Failed to create thread";
        return result;
    }
    
    // Wait for thread
    WaitForSingleObject(hThread, INFINITE);
    
    // Get return value (module base)
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    if (exitCode == 0) {
        result.error = "LoadLibrary returned NULL";
        return result;
    }
    
    result.success = true;
    result.moduleBase = exitCode;
    return result;
}

InjectionResult Injector::InjectManualMap(uint32_t pid, const std::string& dllPath) {
    // Manual mapping is more complex - this is a stub
    // Full implementation requires:
    // 1. Read PE file
    // 2. Allocate memory in target
    // 3. Map sections
    // 4. Fix relocations
    // 5. Resolve imports
    // 6. Execute TLS callbacks
    // 7. Call DllMain
    
    InjectionResult result = {false, "Manual mapping not implemented - use LoadLibrary", 0};
    
    // For production, implement full PE loader or use existing library
    // Falling back to LoadLibrary for now
    return InjectLoadLibrary(pid, dllPath);
}

InjectionResult Injector::InjectThreadHijack(uint32_t pid, const std::string& dllPath) {
    InjectionResult result = {false, "Thread hijack not implemented", 0};
    return result;
}

InjectionResult Injector::InjectAPC(uint32_t pid, const std::string& dllPath) {
    InjectionResult result = {false, "APC injection not implemented", 0};
    return result;
}

InjectionResult Injector::InjectNtCreateThread(uint32_t pid, const std::string& dllPath) {
    InjectionResult result = {false, "NtCreateThreadEx injection not implemented", 0};
    return result;
}

// ============================================
// Public Methods
// ============================================

InjectionResult Injector::Inject(const std::string& processName, const std::string& dllPath,
                                  InjectionMethod method) {
    uint32_t pid = FindProcessId(processName);
    if (pid == 0) {
        return {false, "Process not found: " + processName, 0};
    }
    
    return InjectByPid(pid, dllPath, method);
}

InjectionResult Injector::InjectByPid(uint32_t pid, const std::string& dllPath,
                                       InjectionMethod method) {
    // Check if file exists
    std::ifstream file(dllPath);
    if (!file.good()) {
        return {false, "DLL file not found: " + dllPath, 0};
    }
    file.close();
    
    switch (method) {
        case InjectionMethod::LoadLibrary:
            return InjectLoadLibrary(pid, dllPath);
        case InjectionMethod::ManualMap:
            return InjectManualMap(pid, dllPath);
        case InjectionMethod::ThreadHijack:
            return InjectThreadHijack(pid, dllPath);
        case InjectionMethod::QueueUserAPC:
            return InjectAPC(pid, dllPath);
        case InjectionMethod::NtCreateThreadEx:
            return InjectNtCreateThread(pid, dllPath);
        default:
            return InjectLoadLibrary(pid, dllPath);
    }
}

InjectionResult Injector::WaitAndInject(const std::string& processName, const std::string& dllPath,
                                         InjectionMethod method, uint32_t timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        uint32_t pid = FindProcessId(processName);
        if (pid != 0) {
            // Wait a bit for process to initialize
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return InjectByPid(pid, dllPath, method);
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        
        if (elapsed >= timeoutMs) {
            return {false, "Timeout waiting for process", 0};
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Injector::IsInjected(const std::string& processName, const std::string& dllName) {
    uint32_t pid = FindProcessId(processName);
    if (pid == 0) return false;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    MODULEENTRY32 entry;
    entry.dwSize = sizeof(entry);
    
    bool found = false;
    
    if (Module32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szModule, dllName.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Module32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return found;
}

bool Injector::Eject(const std::string& processName, const std::string& dllName) {
    uint32_t pid = FindProcessId(processName);
    if (pid == 0) return false;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    MODULEENTRY32 entry;
    entry.dwSize = sizeof(entry);
    
    HMODULE hModule = nullptr;
    
    if (Module32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szModule, dllName.c_str()) == 0) {
                hModule = entry.hModule;
                break;
            }
        } while (Module32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    
    if (!hModule) return false;
    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION,
        FALSE, pid
    );
    
    if (!hProcess) return false;
    
    LPVOID freeLibrary = (LPVOID)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "FreeLibrary");
    
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)freeLibrary, hModule, 0, nullptr);
    
    if (hThread) {
        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
    }
    
    CloseHandle(hProcess);
    return hThread != nullptr;
}

#else
// Linux stubs
uint32_t Injector::FindProcessId(const std::string& processName) { return 0; }
InjectionResult Injector::Inject(const std::string&, const std::string&, InjectionMethod) { 
    return {false, "Not implemented on Linux", 0}; 
}
InjectionResult Injector::InjectByPid(uint32_t, const std::string&, InjectionMethod) { 
    return {false, "Not implemented on Linux", 0}; 
}
InjectionResult Injector::WaitAndInject(const std::string&, const std::string&, InjectionMethod, uint32_t) { 
    return {false, "Not implemented on Linux", 0}; 
}
bool Injector::IsInjected(const std::string&, const std::string&) { return false; }
bool Injector::Eject(const std::string&, const std::string&) { return false; }
InjectionResult Injector::InjectLoadLibrary(uint32_t, const std::string&) { return {false, "", 0}; }
InjectionResult Injector::InjectManualMap(uint32_t, const std::string&) { return {false, "", 0}; }
InjectionResult Injector::InjectThreadHijack(uint32_t, const std::string&) { return {false, "", 0}; }
InjectionResult Injector::InjectAPC(uint32_t, const std::string&) { return {false, "", 0}; }
InjectionResult Injector::InjectNtCreateThread(uint32_t, const std::string&) { return {false, "", 0}; }
#endif

} // namespace launcher

