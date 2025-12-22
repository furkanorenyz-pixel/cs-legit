/**
 * HYPERVISOR CHEAT - Main Entry Point
 * CS2 ESP with Ring -1 Hypervisor backend
 */

#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <iostream>
#include <thread>
#include <atomic>

#include "../include/hypervisor.hpp"
#include "../include/esp.hpp"

// ImGui (would be included from submodule)
// #include <imgui.h>
// #include <imgui_impl_win32.h>
// #include <imgui_impl_dx11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

// ============================================
// Globals
// ============================================

static std::atomic<bool> g_running{true};
static HWND g_overlayWindow = nullptr;
static HWND g_gameWindow = nullptr;

// DirectX
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static IDXGISwapChain* g_swapchain = nullptr;
static ID3D11RenderTargetView* g_renderTarget = nullptr;

// Menu
static bool g_menuVisible = false;

// ============================================
// Window Procedure
// ============================================

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Forward to ImGui
    // if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    //     return true;
    
    switch (msg) {
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_INSERT) {
                g_menuVisible = !g_menuVisible;
            }
            if (wParam == VK_END) {
                g_running = false;
            }
            break;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================
// Overlay Creation
// ============================================

bool CreateOverlayWindow() {
    // Register class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"HypervisorOverlay";
    
    if (!RegisterClassEx(&wc)) {
        return false;
    }
    
    // Get screen size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Create window
    g_overlayWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    if (!g_overlayWindow) {
        return false;
    }
    
    // Make transparent
    SetLayeredWindowAttributes(g_overlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // Extend frame for DWM composition
    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(g_overlayWindow, &margins);
    
    ShowWindow(g_overlayWindow, SW_SHOW);
    UpdateWindow(g_overlayWindow);
    
    return true;
}

// ============================================
// DirectX Initialization
// ============================================

bool InitDirectX() {
    RECT rect;
    GetClientRect(g_overlayWindow, &rect);
    
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = rect.right - rect.left;
    sd.BufferDesc.Height = rect.bottom - rect.top;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_overlayWindow;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &sd,
        &g_swapchain,
        &g_device,
        &featureLevel,
        &g_context
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create render target
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
    backBuffer->Release();
    
    return true;
}

void CleanupDirectX() {
    if (g_renderTarget) g_renderTarget->Release();
    if (g_swapchain) g_swapchain->Release();
    if (g_context) g_context->Release();
    if (g_device) g_device->Release();
}

// ============================================
// Menu Rendering
// ============================================

void RenderMenu() {
    if (!g_menuVisible) return;
    
    // ImGui menu would go here
    // For now, just a placeholder
    
    /*
    ImGui::Begin("HYPERVISOR CHEAT", &g_menuVisible, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Ring -1 Hypervisor Active!");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& cfg = esp::Config();
        
        ImGui::Checkbox("Enable ESP", &cfg.enabled);
        ImGui::Checkbox("Box", &cfg.boxEnabled);
        ImGui::Checkbox("Health Bar", &cfg.healthBarEnabled);
        ImGui::Checkbox("Name", &cfg.nameEnabled);
        ImGui::Checkbox("Distance", &cfg.distanceEnabled);
        
        ImGui::SliderFloat("Max Distance", &cfg.maxDistance, 100.0f, 1000.0f);
        
        const char* boxStyles[] = {"2D", "2D Corner", "3D"};
        ImGui::Combo("Box Style", &cfg.boxStyle, boxStyles, 3);
        
        ImGui::ColorEdit4("Enemy Color", &cfg.enemyColor.r);
        ImGui::ColorEdit4("Enemy Visible", &cfg.enemyVisibleColor.r);
    }
    
    ImGui::Separator();
    ImGui::Text("Press INSERT to toggle menu");
    ImGui::Text("Press END to exit");
    
    ImGui::End();
    */
}

// ============================================
// Main Loop
// ============================================

void MainLoop() {
    MSG msg = {};
    
    while (g_running) {
        // Process Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            if (msg.message == WM_QUIT) {
                g_running = false;
            }
        }
        
        if (!g_running) break;
        
        // Follow game window
        g_gameWindow = FindWindowA(nullptr, "Counter-Strike 2");
        if (g_gameWindow) {
            RECT rect;
            GetWindowRect(g_gameWindow, &rect);
            SetWindowPos(g_overlayWindow, HWND_TOPMOST,
                rect.left, rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOACTIVATE);
        }
        
        // Update ESP data from hypervisor
        esp::Update();
        
        // Clear screen
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        g_context->ClearRenderTargetView(g_renderTarget, clearColor);
        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        
        // Render ESP
        esp::Render();
        
        // Render menu
        RenderMenu();
        
        // Present
        g_swapchain->Present(1, 0);  // VSync
        
        // Sleep a bit to not hog CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============================================
// Entry Point
// ============================================

int main() {
    // Set console title
    SetConsoleTitleA("HYPERVISOR CHEAT - CS2 ESP");
    
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════════════════════╗
║                                                                               ║
║   ██╗  ██╗██╗   ██╗██████╗ ███████╗██████╗ ██╗   ██╗██╗███████╗ ██████╗ ██████╗║
║   ██║  ██║╚██╗ ██╔╝██╔══██╗██╔════╝██╔══██╗██║   ██║██║██╔════╝██╔═══██╗██╔══██║
║   ███████║ ╚████╔╝ ██████╔╝█████╗  ██████╔╝██║   ██║██║███████╗██║   ██║██████╔║
║   ██╔══██║  ╚██╔╝  ██╔═══╝ ██╔══╝  ██╔══██╗╚██╗ ██╔╝██║╚════██║██║   ██║██╔══██║
║   ██║  ██║   ██║   ██║     ███████╗██║  ██║ ╚████╔╝ ██║███████║╚██████╔╝██║  ██║
║   ╚═╝  ╚═╝   ╚═╝   ╚═╝     ╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝
║                                                                               ║
║                         CS2 ESP - Ring -1 Hypervisor                          ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;

    // Check hypervisor
    std::cout << "[*] Checking hypervisor..." << std::endl;
    
    if (!hv::Init()) {
        std::cout << "[!] Hypervisor not active!" << std::endl;
        std::cout << "[!] Make sure bootkit is installed and system was rebooted." << std::endl;
        std::cout << std::endl;
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
    
    auto info = hv::Hypervisor::Get().GetInfo();
    std::cout << "[+] Hypervisor active!" << std::endl;
    std::cout << "[+] Version: " << (info.Version >> 16) << "." 
              << ((info.Version >> 8) & 0xFF) << "." 
              << (info.Version & 0xFF) << std::endl;
    std::cout << "[+] Signature: " << info.Signature << std::endl;
    std::cout << std::endl;
    
    // Wait for CS2
    std::cout << "[*] Waiting for CS2..." << std::endl;
    
    while (!FindWindowA(nullptr, "Counter-Strike 2")) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            std::cout << "[!] Exit requested." << std::endl;
            return 0;
        }
    }
    
    std::cout << "[+] CS2 found!" << std::endl;
    std::cout << std::endl;
    
    // Create overlay
    std::cout << "[*] Creating overlay..." << std::endl;
    
    if (!CreateOverlayWindow()) {
        std::cout << "[!] Failed to create overlay window!" << std::endl;
        return 1;
    }
    
    // Init DirectX
    std::cout << "[*] Initializing DirectX..." << std::endl;
    
    if (!InitDirectX()) {
        std::cout << "[!] Failed to initialize DirectX!" << std::endl;
        return 1;
    }
    
    // Init ESP
    std::cout << "[*] Initializing ESP..." << std::endl;
    
    if (!esp::Init(g_overlayWindow)) {
        std::cout << "[!] Failed to initialize ESP!" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  READY!                                                   ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  INSERT = Toggle Menu                                     ║" << std::endl;
    std::cout << "║  END    = Exit                                            ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Main loop
    MainLoop();
    
    // Cleanup
    CleanupDirectX();
    DestroyWindow(g_overlayWindow);
    
    std::cout << "[*] Goodbye!" << std::endl;
    
    return 0;
}

