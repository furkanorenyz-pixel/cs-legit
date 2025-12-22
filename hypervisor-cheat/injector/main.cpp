/**
 * HYPERVISOR CHEAT - Manual Map Injector
 * Injects internal DLL into CS2 process
 * 
 * Usage: injector.exe <path_to_dll>
 */

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// ============================================
// Helpers
// ============================================

DWORD GetProcessIdByName(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return 0;
}

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    
    size_t size = file.tellg();
    file.seekg(0);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    return buffer;
}

// ============================================
// Simple LoadLibrary Injection
// ============================================

bool InjectLoadLibrary(DWORD pid, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[!] Failed to open process: " << GetLastError() << std::endl;
        return false;
    }
    
    // Allocate memory for DLL path
    size_t pathLen = dllPath.length() + 1;
    void* remotePath = VirtualAllocEx(hProcess, nullptr, pathLen, 
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) {
        std::cerr << "[!] VirtualAllocEx failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    // Write DLL path
    if (!WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathLen, nullptr)) {
        std::cerr << "[!] WriteProcessMemory failed: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Get LoadLibraryA address
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    void* loadLibrary = GetProcAddress(kernel32, "LoadLibraryA");
    
    // Create remote thread
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)loadLibrary,
                                         remotePath, 0, nullptr);
    if (!hThread) {
        std::cerr << "[!] CreateRemoteThread failed: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Wait for completion
    WaitForSingleObject(hThread, 10000);
    
    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    return true;
}

// ============================================
// Manual Map Injection (Stealthier)
// ============================================

struct ManualMapData {
    void* ImageBase;
    void* NtFlushInstructionCache;
    void* LdrLoadDll;
    void* LdrGetProcedureAddress;
    void* RtlInitAnsiString;
};

bool InjectManualMap(DWORD pid, const std::vector<uint8_t>& dllData) {
    if (dllData.size() < sizeof(IMAGE_DOS_HEADER)) {
        std::cerr << "[!] Invalid DLL data" << std::endl;
        return false;
    }
    
    auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(dllData.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "[!] Invalid DOS signature" << std::endl;
        return false;
    }
    
    auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(dllData.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "[!] Invalid NT signature" << std::endl;
        return false;
    }
    
    if (ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        std::cerr << "[!] Not a 64-bit DLL" << std::endl;
        return false;
    }
    
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[!] Failed to open process: " << GetLastError() << std::endl;
        return false;
    }
    
    // Allocate memory for the DLL
    size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    void* remoteBase = VirtualAllocEx(hProcess, nullptr, imageSize,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteBase) {
        std::cerr << "[!] VirtualAllocEx failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    std::cout << "[+] Allocated " << imageSize << " bytes at 0x" << std::hex << remoteBase << std::dec << std::endl;
    
    // Copy headers
    if (!WriteProcessMemory(hProcess, remoteBase, dllData.data(), 
                            ntHeaders->OptionalHeader.SizeOfHeaders, nullptr)) {
        std::cerr << "[!] Failed to write headers" << std::endl;
        VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Copy sections
    auto* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        if (sectionHeader[i].SizeOfRawData == 0) continue;
        
        void* sectionDest = reinterpret_cast<uint8_t*>(remoteBase) + sectionHeader[i].VirtualAddress;
        const void* sectionSrc = dllData.data() + sectionHeader[i].PointerToRawData;
        
        if (!WriteProcessMemory(hProcess, sectionDest, sectionSrc, 
                                sectionHeader[i].SizeOfRawData, nullptr)) {
            std::cerr << "[!] Failed to write section " << i << std::endl;
            VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }
        
        std::cout << "[+] Wrote section: " << reinterpret_cast<const char*>(sectionHeader[i].Name) << std::endl;
    }
    
    // TODO: Process relocations and imports
    // For now, fall back to LoadLibrary for simplicity
    
    std::cout << "[!] Manual map requires shellcode - using LoadLibrary fallback" << std::endl;
    VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    return false;
}

// ============================================
// Main
// ============================================

int main(int argc, char* argv[]) {
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║           HYPERVISOR CHEAT - DLL INJECTOR                 ║
    ║                    Ring -1 Internal                       ║
    ╚═══════════════════════════════════════════════════════════╝
    )" << std::endl;
    
    std::string dllPath;
    
    if (argc >= 2) {
        dllPath = argv[1];
    } else {
        dllPath = "hv_internal.dll";
        std::cout << "[*] Using default DLL: " << dllPath << std::endl;
    }
    
    // Check DLL exists
    std::ifstream test(dllPath);
    if (!test.good()) {
        std::cerr << "[!] DLL not found: " << dllPath << std::endl;
        std::cerr << "[*] Usage: injector.exe <path_to_dll>" << std::endl;
        return 1;
    }
    test.close();
    
    // Get absolute path
    char fullPath[MAX_PATH];
    GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullPath, nullptr);
    dllPath = fullPath;
    
    std::cout << "[*] DLL: " << dllPath << std::endl;
    
    // Wait for CS2
    std::cout << "[*] Waiting for cs2.exe..." << std::endl;
    
    DWORD pid = 0;
    while (!pid) {
        pid = GetProcessIdByName(L"cs2.exe");
        if (!pid) {
            Sleep(1000);
        }
    }
    
    std::cout << "[+] Found cs2.exe (PID: " << pid << ")" << std::endl;
    
    // Wait for game to fully load
    std::cout << "[*] Waiting 5 seconds for game to initialize..." << std::endl;
    Sleep(5000);
    
    // Inject
    std::cout << "[*] Injecting..." << std::endl;
    
    if (InjectLoadLibrary(pid, dllPath)) {
        std::cout << "[+] Injection successful!" << std::endl;
        std::cout << "[*] Press INSERT in-game to toggle menu" << std::endl;
    } else {
        std::cerr << "[!] Injection failed!" << std::endl;
        return 1;
    }
    
    std::cout << "\n[*] Press ENTER to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}

