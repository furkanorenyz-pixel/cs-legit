/**
 * HYPERVISOR CHEAT - Internal DLL
 * Injected version with VMT hooks + ImGui
 * 
 * This version reads memory directly (no VMCALL needed)
 * Uses VMT hook on Present for rendering
 */

#include <Windows.h>
#include <thread>
#include <chrono>

#include "../include/hooks.hpp"
#include "../include/esp.hpp"

// ============================================
// Globals
// ============================================

static HMODULE g_hModule = nullptr;
static bool g_running = true;

// ============================================
// Main Thread
// ============================================

void MainThread(HMODULE hModule) {
    // Wait for game to fully load
    while (!GetModuleHandleA("client.dll")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Additional wait for stability
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Optional: Allocate console for debugging
#ifdef _DEBUG
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    printf("[HV-Internal] Initializing...\n");
#endif
    
    // Initialize ESP
    esp::Initialize();
    
    // Initialize hooks
    if (!hooks::Initialize()) {
#ifdef _DEBUG
        printf("[HV-Internal] Failed to initialize hooks!\n");
#endif
        FreeLibraryAndExitThread(hModule, 0);
        return;
    }
    
#ifdef _DEBUG
    printf("[HV-Internal] Ready! Press END to unload.\n");
#endif
    
    // Main loop - wait for exit key
    while (g_running) {
        if (GetAsyncKeyState(VK_END) & 1) {
            g_running = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cleanup
#ifdef _DEBUG
    printf("[HV-Internal] Unloading...\n");
#endif
    
    hooks::Shutdown();
    esp::Shutdown();
    
#ifdef _DEBUG
    fclose(f);
    FreeConsole();
#endif
    
    FreeLibraryAndExitThread(hModule, 0);
}

// ============================================
// DLL Entry Point
// ============================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_hModule = hModule;
        
        // Create main thread
        HANDLE hThread = CreateThread(
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread),
            hModule,
            0,
            nullptr
        );
        
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    
    return TRUE;
}

