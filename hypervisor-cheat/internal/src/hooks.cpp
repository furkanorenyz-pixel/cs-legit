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

// ImGui headers - adjust path as needed
// For now, we'll use conditional compilation
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

// Present function pointer
using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

static PresentFn g_originalPresent = nullptr;
static ResizeBuffersFn g_originalResizeBuffers = nullptr;
static void* g_swapChainVTable[18] = {};

// ============================================
// Window Procedure Hook
// ============================================

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef HAS_IMGUI
    if (g_initialized && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }
#endif
    
    // Toggle menu
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_menuOpen = !g_menuOpen;
        return 0;
    }
    
    // Block input when menu is open
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
    
    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// ============================================
// Render Menu
// ============================================

void RenderMenu() {
#ifdef HAS_IMGUI
    if (!g_menuOpen) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(_XS("HYPERVISOR CHEAT - Internal"), &g_menuOpen, ImGuiWindowFlags_NoCollapse);
    
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), _XS("Ring -1 Hypervisor Internal"));
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader(_XS("ESP"), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& cfg = esp::g_config;
        
        ImGui::Checkbox(_XS("Enable ESP"), &cfg.enabled);
        
        if (cfg.enabled) {
            ImGui::Indent();
            
            ImGui::Checkbox(_XS("Box"), &cfg.boxEnabled);
            if (cfg.boxEnabled) {
                const char* styles[] = {_XS("2D"), _XS("Corner")};
                ImGui::Combo(_XS("Box Style"), &cfg.boxStyle, styles, 2);
                ImGui::SliderFloat(_XS("Thickness"), &cfg.boxThickness, 1.0f, 5.0f);
            }
            
            ImGui::Checkbox(_XS("Health Bar"), &cfg.healthBarEnabled);
            ImGui::Checkbox(_XS("Name"), &cfg.nameEnabled);
            ImGui::Checkbox(_XS("Distance"), &cfg.distanceEnabled);
            ImGui::Checkbox(_XS("Skeleton"), &cfg.skeletonEnabled);
            ImGui::Checkbox(_XS("Head Dot"), &cfg.headDotEnabled);
            ImGui::Checkbox(_XS("Snapline"), &cfg.snaplineEnabled);
            
            ImGui::SliderFloat(_XS("Max Distance"), &cfg.maxDistance, 100.0f, 1000.0f);
            
            ImGui::Unindent();
        }
    }
    
    if (ImGui::CollapsingHeader(_XS("Colors"))) {
        auto& cfg = esp::g_config;
        
        ImGui::ColorEdit4(_XS("Enemy"), cfg.enemyColor);
        ImGui::ColorEdit4(_XS("Enemy Visible"), cfg.enemyVisibleColor);
        ImGui::ColorEdit4(_XS("Team"), cfg.teamColor);
        ImGui::ColorEdit4(_XS("Health"), cfg.healthColor);
        ImGui::ColorEdit4(_XS("Name"), cfg.nameColor);
        ImGui::ColorEdit4(_XS("Skeleton"), cfg.skeletonColor);
    }
    
    ImGui::Separator();
    ImGui::Text(_XS("Press INSERT to toggle menu"));
    ImGui::Text(_XS("Press END to unload"));
    
    ImGui::End();
#else
    // Fallback: no ImGui, just draw with GDI or skip menu
#endif
}

// ============================================
// Present Hook
// ============================================

HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    // First time initialization
    if (!g_initialized) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) {
            g_device->GetImmediateContext(&g_context);
            
            // Get window
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_hwnd = desc.OutputWindow;
            
            // Create render target
            ID3D11Texture2D* backBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
                g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
                backBuffer->Release();
            }
            
            // Hook WndProc
            g_originalWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
            
#ifdef HAS_IMGUI
            // Initialize ImGui
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr;
            
            // Style
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 8.0f;
            style.FrameRounding = 4.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            
            // Initialize backends
            ImGui_ImplWin32_Init(g_hwnd);
            ImGui_ImplDX11_Init(g_device, g_context);
#endif
            
            // Initialize ESP
            esp::Initialize();
            
            g_initialized = true;
        }
    }
    
    if (g_initialized) {
        // Update ESP data
        esp::Update();
        
#ifdef HAS_IMGUI
        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Render ESP
        esp::Render();
        
        // Render menu
        RenderMenu();
        
        // End ImGui frame
        ImGui::Render();
        
        // Set render target
        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        
        // Draw ImGui
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
    }
    
    // Call original
    return g_originalPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================
// ResizeBuffers Hook (handle window resize)
// ============================================

HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, 
                                    UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    // Release render target before resize
    if (g_renderTarget) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
    
#ifdef HAS_IMGUI
    ImGui_ImplDX11_InvalidateDeviceObjects();
#endif
    
    // Call original
    HRESULT hr = g_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    // Recreate render target
    if (SUCCEEDED(hr) && g_device) {
        ID3D11Texture2D* backBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
            backBuffer->Release();
        }
        
#ifdef HAS_IMGUI
        ImGui_ImplDX11_CreateDeviceObjects();
#endif
    }
    
    return hr;
}

// ============================================
// Get SwapChain VTable
// ============================================

bool GetSwapChainVTable() {
    // Create dummy window
    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0, 
                      GetModuleHandleW(nullptr), nullptr, nullptr, nullptr, 
                      nullptr, L"DummyDX11", nullptr};
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 
                              0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    
    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
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
    
    // Copy vtable
    void** vTable = *reinterpret_cast<void***>(swapChain);
    memcpy(g_swapChainVTable, vTable, sizeof(g_swapChainVTable));
    
    // Cleanup
    swapChain->Release();
    device->Release();
    context->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    
    return true;
}

// ============================================
// MinHook-style Trampoline Hook
// ============================================

bool CreateHook(void* target, void* detour, void** original) {
    // Simple hook using VirtualProtect
    // For production, use MinHook or Detours
    
    DWORD oldProtect;
    if (!VirtualProtect(target, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // Save original bytes for trampoline
    static uint8_t trampoline[64];
    static size_t trampolineOffset = 0;
    
    uint8_t* trampolinePtr = trampoline + trampolineOffset;
    trampolineOffset += 32;
    
    // Copy original bytes (assume 14-byte instruction for x64 jump)
    memcpy(trampolinePtr, target, 14);
    
    // Add jump back
    trampolinePtr[14] = 0xFF;
    trampolinePtr[15] = 0x25;
    *reinterpret_cast<uint32_t*>(trampolinePtr + 16) = 0;
    *reinterpret_cast<uint64_t*>(trampolinePtr + 20) = reinterpret_cast<uint64_t>(target) + 14;
    
    // Make trampoline executable
    DWORD temp;
    VirtualProtect(trampolinePtr, 32, PAGE_EXECUTE_READWRITE, &temp);
    
    *original = trampolinePtr;
    
    // Write jump to detour
    uint8_t* hook = reinterpret_cast<uint8_t*>(target);
    hook[0] = 0xFF;
    hook[1] = 0x25;
    *reinterpret_cast<uint32_t*>(hook + 2) = 0;
    *reinterpret_cast<uint64_t*>(hook + 6) = reinterpret_cast<uint64_t>(detour);
    
    VirtualProtect(target, 16, oldProtect, &temp);
    
    return true;
}

// ============================================
// Initialize
// ============================================

bool Initialize() {
    // Get SwapChain vtable
    if (!GetSwapChainVTable()) {
        return false;
    }
    
    // Wait for game window
    HWND gameWindow = nullptr;
    for (int i = 0; i < 100 && !gameWindow; ++i) {
        gameWindow = FindWindowA("SDL_app", nullptr);  // CS2 window class
        if (!gameWindow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    if (!gameWindow) {
        return false;
    }
    
    // Hook Present (index 8) and ResizeBuffers (index 13)
    g_originalPresent = reinterpret_cast<PresentFn>(g_swapChainVTable[8]);
    g_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(g_swapChainVTable[13]);
    
    // Create hooks
    void* originalPresent = nullptr;
    void* originalResize = nullptr;
    
    if (!CreateHook(g_swapChainVTable[8], HookedPresent, &originalPresent)) {
        return false;
    }
    g_originalPresent = reinterpret_cast<PresentFn>(originalPresent);
    
    if (!CreateHook(g_swapChainVTable[13], HookedResizeBuffers, &originalResize)) {
        return false;
    }
    g_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(originalResize);
    
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
    // Cleanup ImGui
    if (g_initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
#endif
    
    // Release render target
    if (g_renderTarget) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
    
    // Cleanup ESP
    esp::Shutdown();
    
    g_initialized = false;
}

} // namespace hooks
