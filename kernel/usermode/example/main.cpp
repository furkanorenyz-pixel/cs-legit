/*
 * KERNEL BYPASS FRAMEWORK
 * Example Usage
 */

#include <kernel_interface.hpp>
#include <stdio.h>

int main() {
    printf("===========================================\n");
    printf("  Kernel Bypass Framework - Example\n");
    printf("===========================================\n\n");
    
    // Create interface
    kb::KernelInterface ki;
    
    // Initialize with auto-detection
    printf("[*] Initializing kernel interface...\n");
    
    if (!ki.Initialize()) {
        printf("[!] Failed to initialize any backend!\n");
        printf("[!] Make sure driver is loaded or run as admin.\n");
        return 1;
    }
    
    printf("[+] Initialized successfully!\n");
    printf("[+] Backend: %s\n", ki.GetBackendName());
    printf("[+] Ring Level: %d\n", (int)ki.GetRingLevel());
    printf("\n");
    
    // Find CS2 process
    printf("[*] Looking for cs2.exe...\n");
    DWORD pid = ki.FindProcess("cs2.exe");
    
    if (pid == 0) {
        printf("[!] CS2 not found. Start the game first.\n");
        return 1;
    }
    
    printf("[+] Found CS2 PID: %lu\n", pid);
    
    // Get process info
    auto info = ki.GetProcessInfo(pid);
    if (info) {
        printf("[+] Process: %s\n", info->name.c_str());
        printf("[+] Base: 0x%llX\n", info->baseAddress);
        printf("[+] Is WoW64: %s\n", info->isWow64 ? "Yes" : "No");
    }
    
    // Get client.dll base
    printf("\n[*] Looking for client.dll...\n");
    auto clientBase = ki.GetModuleBase(pid, "client.dll");
    
    if (!clientBase) {
        printf("[!] client.dll not found. Is CS2 fully loaded?\n");
        return 1;
    }
    
    printf("[+] client.dll base: 0x%llX\n", *clientBase);
    
    // Example: Read entity list (offset from cs2-dumper)
    constexpr ULONG64 dwEntityList = 0x1D13CE8;
    
    printf("\n[*] Reading EntityList...\n");
    ULONG64 entityList = ki.Read<ULONG64>(pid, *clientBase + dwEntityList);
    printf("[+] EntityList: 0x%llX\n", entityList);
    
    // Example: Read local player controller
    constexpr ULONG64 dwLocalPlayerController = 0x1E1DC18;
    
    ULONG64 localController = ki.Read<ULONG64>(pid, *clientBase + dwLocalPlayerController);
    printf("[+] LocalPlayerController: 0x%llX\n", localController);
    
    // If kernel driver is available, try advanced features
    if (ki.GetRingLevel() <= RING_LEVEL_KERNEL) {
        printf("\n[*] Testing advanced features (Ring 0+)...\n");
        
        // Hide our process (DKOM)
        // WARNING: This can cause instability!
        // ki.HideProcess(GetCurrentProcessId());
        
        // Randomize HWID
        // ki.RandomizeAllHwid();
        
        printf("[+] Advanced features available but not demonstrated.\n");
        printf("[+] Uncomment in code to use (at your own risk).\n");
    }
    
    printf("\n===========================================\n");
    printf("  Example completed successfully!\n");
    printf("===========================================\n");
    
    return 0;
}

