/**
 * HYPERVISOR CHEAT - Internal Hooks Implementation
 * VMT Hook for Present + ImGui initialization
 */

#include "../include/hooks.hpp"
#include "../include/esp.hpp"
#include "../include/memory.hpp"
#include "../include/xorstr.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <mutex>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef HAS_IMGUI
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace hooks {

// ============================================
// Globals
// ============================================

static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static ID3D11RenderTargetView* g_renderTarget = nullptr;
static HWND g_hwnd = nullptr;
static WNDPROC g_originalWndProc = nullptr;
static bool g_initialized = false;
static bool g_menuOpen = true;
static std::mutex g_mutex;

// Hook data
using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

static PresentFn g_originalPresent = nullptr;
static ResizeBuffersFn g_originalResizeBuffers = nullptr;

// For VMT hook
static uintptr_t* g_pSwapChainVTable = nullptr;
static uintptr_t g_originalPresentAddr = 0;
static uintptr_t g_originalResizeAddr = 0;

// ============================================
// Window Procedure Hook
// ============================================

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef HAS_IMGUI
    if (g_initialized && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }
#endif
    
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_menuOpen = !g_menuOpen;
        return 0;
    }
    
#ifdef HAS_IMGUI
    if (g_menuOpen && g_initialized) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:
                case WM_MOUSEMOVE:
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_CHAR:
                    return 0;
            }
        }
    }
#endif
    
    return CallWindowProcW(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// ============================================
// Render Menu
// ============================================

void RenderMenu() {
#ifdef HAS_IMGUI
    if (!g_menuOpen) return;
    
    ImGui::SetNextWindowSize(ImVec2(420, 520), ImGuiCond_FirstUseEver);
    ImGui::Begin("HYPERVISOR CHEAT", &g_menuOpen, ImGuiWindowFlags_NoCollapse);
    
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Ring -1 Internal Cheat");
    ImGui::Text("CS2 ESP + Menu");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("ESP Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& cfg = esp::g_config;
        
        ImGui::Checkbox("Enable ESP", &cfg.enabled);
        
        if (cfg.enabled) {
            ImGui::Indent();
            
            ImGui::Checkbox("Box", &cfg.boxEnabled);
            if (cfg.boxEnabled) {
                const char* styles[] = {"2D Box", "Corner Box"};
                ImGui::Combo("Style", &cfg.boxStyle, styles, 2);
                ImGui::SliderFloat("Thickness", &cfg.boxThickness, 1.0f, 5.0f);
            }
            
            ImGui::Checkbox("Health Bar", &cfg.healthBarEnabled);
            ImGui::Checkbox("Player Name", &cfg.nameEnabled);
            ImGui::Checkbox("Distance", &cfg.distanceEnabled);
            ImGui::Checkbox("Skeleton", &cfg.skeletonEnabled);
            ImGui::Checkbox("Head Dot", &cfg.headDotEnabled);
            ImGui::Checkbox("Snaplines", &cfg.snaplineEnabled);
            
            ImGui::SliderFloat("Max Distance", &cfg.maxDistance, 50.0f, 1000.0f, "%.0f m");
            
            ImGui::Unindent();
        }
    }
    
    if (ImGui::CollapsingHeader("Colors")) {
        auto& cfg = esp::g_config;
        
        ImGui::ColorEdit4("Enemy", cfg.enemyColor, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Enemy (Visible)", cfg.enemyVisibleColor, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Team", cfg.teamColor, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Skeleton", cfg.skeletonColor, ImGuiColorEditFlags_NoInputs);
    }
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "INSERT - Toggle Menu");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "END - Unload Cheat");
    
    ImGui::End();
#endif
}

// ============================================
// Present Hook
// ============================================

HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) {
            g_device->GetImmediateContext(&g_context);
            
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_hwnd = desc.OutputWindow;
            
            ID3D11Texture2D* backBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
                g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
                backBuffer->Release();
            }
            
            g_originalWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
            
#ifdef HAS_IMGUI
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr;
            
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 8.0f;
            style.FrameRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            
            // Orange theme
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.6f, 0.2f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.6f, 0.2f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.6f, 0.2f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);
            
            ImGui_ImplWin32_Init(g_hwnd);
            ImGui_ImplDX11_Init(g_device, g_context);
#endif
            
            esp::Initialize();
            g_initialized = true;
        }
    }
    
    if (g_initialized) {
        esp::Update();
        
#ifdef HAS_IMGUI
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        esp::Render();
        RenderMenu();
        
        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
    }
    
    return g_originalPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================
// ResizeBuffers Hook
// ============================================

HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, 
                                    UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_renderTarget) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
    
#ifdef HAS_IMGUI
    if (g_initialized) {
        ImGui_ImplDX11_InvalidateDeviceObjects();
    }
#endif
    
    HRESULT hr = g_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    if (SUCCEEDED(hr) && g_device) {
        ID3D11Texture2D* backBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
            backBuffer->Release();
        }
        
#ifdef HAS_IMGUI
        if (g_initialized) {
            ImGui_ImplDX11_CreateDeviceObjects();
        }
#endif
    }
    
    return hr;
}

// ============================================
// Initialize - Get SwapChain and Hook
// ============================================

bool Initialize() {
    // Create dummy window and device to get vtable
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"HV_DX11";
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 
                              0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 2;
    sd.BufferDesc.Height = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &swapChain, &device, &featureLevel, &context
    );
    
    if (FAILED(hr)) {
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    
    // Get vtable addresses
    void** vTable = *reinterpret_cast<void***>(swapChain);
    g_originalPresent = reinterpret_cast<PresentFn>(vTable[8]);
    g_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(vTable[13]);
    
    // Save for unhook
    g_originalPresentAddr = reinterpret_cast<uintptr_t>(vTable[8]);
    g_originalResizeAddr = reinterpret_cast<uintptr_t>(vTable[13]);
    
    // Cleanup dummy resources
    swapChain->Release();
    device->Release();
    context->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    
    // Wait for game window
    HWND gameWindow = nullptr;
    for (int i = 0; i < 300 && !gameWindow; ++i) {
        gameWindow = FindWindowA("SDL_app", nullptr);
        if (!gameWindow) {
            Sleep(100);
        }
    }
    
    if (!gameWindow) {
        return false;
    }
    
    // Use detour hooking on the function addresses
    // This hooks ALL swapchains since we're patching the actual function
    
    DWORD oldProtect;
    
    // Hook Present
    VirtualProtect(reinterpret_cast<void*>(g_originalPresentAddr), 14, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    // Create trampoline
    static uint8_t presentTrampoline[32];
    memcpy(presentTrampoline, reinterpret_cast<void*>(g_originalPresentAddr), 14);
    presentTrampoline[14] = 0xFF;
    presentTrampoline[15] = 0x25;
    *reinterpret_cast<uint32_t*>(presentTrampoline + 16) = 0;
    *reinterpret_cast<uint64_t*>(presentTrampoline + 20) = g_originalPresentAddr + 14;
    
    DWORD temp;
    VirtualProtect(presentTrampoline, 32, PAGE_EXECUTE_READWRITE, &temp);
    g_originalPresent = reinterpret_cast<PresentFn>(presentTrampoline);
    
    // Write jump to our hook
    uint8_t* hook = reinterpret_cast<uint8_t*>(g_originalPresentAddr);
    hook[0] = 0xFF;
    hook[1] = 0x25;
    *reinterpret_cast<uint32_t*>(hook + 2) = 0;
    *reinterpret_cast<uint64_t*>(hook + 6) = reinterpret_cast<uint64_t>(HookedPresent);
    
    VirtualProtect(reinterpret_cast<void*>(g_originalPresentAddr), 14, oldProtect, &temp);
    
    // Hook ResizeBuffers similarly
    VirtualProtect(reinterpret_cast<void*>(g_originalResizeAddr), 14, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    static uint8_t resizeTrampoline[32];
    memcpy(resizeTrampoline, reinterpret_cast<void*>(g_originalResizeAddr), 14);
    resizeTrampoline[14] = 0xFF;
    resizeTrampoline[15] = 0x25;
    *reinterpret_cast<uint32_t*>(resizeTrampoline + 16) = 0;
    *reinterpret_cast<uint64_t*>(resizeTrampoline + 20) = g_originalResizeAddr + 14;
    
    VirtualProtect(resizeTrampoline, 32, PAGE_EXECUTE_READWRITE, &temp);
    g_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(resizeTrampoline);
    
    hook = reinterpret_cast<uint8_t*>(g_originalResizeAddr);
    hook[0] = 0xFF;
    hook[1] = 0x25;
    *reinterpret_cast<uint32_t*>(hook + 2) = 0;
    *reinterpret_cast<uint64_t*>(hook + 6) = reinterpret_cast<uint64_t>(HookedResizeBuffers);
    
    VirtualProtect(reinterpret_cast<void*>(g_originalResizeAddr), 14, oldProtect, &temp);
    
    return true;
}

// ============================================
// Shutdown
// ============================================

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    // Restore WndProc
    if (g_originalWndProc && g_hwnd) {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    }
    
#ifdef HAS_IMGUI
    if (g_initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
#endif
    
    if (g_renderTarget) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
    
    esp::Shutdown();
    g_initialized = false;
    
    // Note: Unhooking detours would require restoring original bytes
    // For clean unload, would need to save original 14 bytes
}

} // namespace hooks
