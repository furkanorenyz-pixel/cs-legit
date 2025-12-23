/*
 * CS-LEGIT Premium Launcher v2.0
 * External ESP for CS2
 * 
 * Professional GUI with ImGui + DirectX11
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>
#include <fstream>
#include <atomic>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "shell32.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "api.hpp"

namespace fs = std::filesystem;

// ============================================
// Version
// ============================================
#define LAUNCHER_VERSION "2.0.0"

// ============================================
// App State
// ============================================
enum class Screen { Login, Register, Main };

Screen g_screen = Screen::Login;
char g_username[64] = "";
char g_password[64] = "";
char g_licenseKey[64] = "";
std::string g_error = "";
std::string g_status = "";
std::atomic<bool> g_isLoading{false};
std::atomic<float> g_downloadProgress{0.0f};
std::atomic<bool> g_isDownloading{false};
float g_animTime = 0.0f;

// User data
std::string g_userDisplay = "";
std::string g_licenseStatus = "";
std::string g_cheatVersion = "---";
bool g_hasLicense = false;

// ============================================
// Colors - Premium Purple Theme
// ============================================
namespace colors {
    const ImVec4 bg = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
    const ImVec4 card = ImVec4(0.09f, 0.09f, 0.12f, 1.0f);
    const ImVec4 cardHover = ImVec4(0.12f, 0.12f, 0.16f, 1.0f);
    
    const ImVec4 accent = ImVec4(0.58f, 0.44f, 0.86f, 1.0f);       // Purple
    const ImVec4 accentHover = ImVec4(0.68f, 0.54f, 0.96f, 1.0f);
    const ImVec4 accentDark = ImVec4(0.38f, 0.24f, 0.66f, 1.0f);
    
    const ImVec4 success = ImVec4(0.20f, 0.80f, 0.40f, 1.0f);
    const ImVec4 warning = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
    const ImVec4 danger = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
    
    const ImVec4 text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 textMuted = ImVec4(0.5f, 0.5f, 0.55f, 1.0f);
    const ImVec4 textDim = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
    
    const ImU32 gradientPurple = IM_COL32(147, 112, 219, 255);
    const ImU32 gradientBlue = IM_COL32(100, 149, 237, 255);
}

// ============================================
// DirectX
// ============================================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================
// Helpers
// ============================================
bool IsCS2Running() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"cs2.exe") == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return found;
}

std::string GetLocalVersion() {
    std::ifstream file("version.json");
    if (!file.is_open()) return "";
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Simple parse
    size_t pos = content.find("\"version\"");
    if (pos == std::string::npos) return "";
    
    size_t start = content.find("\"", pos + 10) + 1;
    size_t end = content.find("\"", start);
    if (start == std::string::npos || end == std::string::npos) return "";
    
    return content.substr(start, end - start);
}

void SaveVersion(const std::string& version) {
    std::ofstream file("version.json");
    file << "{\"version\":\"" << version << "\"}";
    file.close();
}

// ============================================
// Auth Functions
// ============================================
void DoLogin() {
    if (g_isLoading) return;
    if (strlen(g_username) == 0 || strlen(g_password) == 0) {
        g_error = "Enter username and password";
        return;
    }
    
    g_isLoading = true;
    g_error = "";
    
    std::thread([]() {
        auto result = api::Client::Get().Login(g_username, g_password);
        
        g_isLoading = false;
        
        if (result.success) {
            g_userDisplay = result.user.username;
            g_hasLicense = api::Client::Get().HasLicense("cs2");
            g_licenseStatus = g_hasLicense ? "Active" : "No license";
            g_cheatVersion = api::Client::Get().GetLatestVersion("cs2");
            g_screen = Screen::Main;
            api::Client::Get().SaveSession();
        } else {
            g_error = result.error.empty() ? "Login failed" : result.error;
        }
    }).detach();
}

void DoRegister() {
    if (g_isLoading) return;
    if (strlen(g_username) == 0 || strlen(g_password) == 0) {
        g_error = "Enter username and password";
        return;
    }
    if (strlen(g_licenseKey) == 0) {
        g_error = "Enter your license key";
        return;
    }
    
    g_isLoading = true;
    g_error = "";
    
    std::thread([]() {
        auto result = api::Client::Get().Register(g_username, g_password, g_licenseKey);
        
        g_isLoading = false;
        
        if (result.success) {
            g_userDisplay = result.user.username;
            g_hasLicense = true;
            g_licenseStatus = "Active";
            g_cheatVersion = api::Client::Get().GetLatestVersion("cs2");
            g_screen = Screen::Main;
            api::Client::Get().SaveSession();
        } else {
            g_error = result.error.empty() ? "Registration failed" : result.error;
        }
    }).detach();
}

void TryAutoLogin() {
    if (api::Client::Get().LoadSession()) {
        if (api::Client::Get().VerifyToken()) {
            g_userDisplay = api::Client::Get().GetUser().username;
            g_hasLicense = api::Client::Get().HasLicense("cs2");
            g_licenseStatus = g_hasLicense ? "Active" : "No license";
            g_cheatVersion = api::Client::Get().GetLatestVersion("cs2");
            g_screen = Screen::Main;
        }
    }
}

void Logout() {
    api::Client::Get().Logout();
    api::Client::Get().ClearSession();
    g_screen = Screen::Login;
    memset(g_username, 0, sizeof(g_username));
    memset(g_password, 0, sizeof(g_password));
    g_error = "";
}

// ============================================
// Launch Function
// ============================================
void LaunchCheat() {
    if (g_isLoading || g_isDownloading) return;
    
    if (!g_hasLicense) {
        g_status = "No license! Contact admin.";
        return;
    }
    
    // Check if CS2 is running
    if (!IsCS2Running()) {
        g_status = "Start CS2 first!";
        return;
    }
    
    std::string exePath = "externa.exe";
    std::string localVersion = GetLocalVersion();
    std::string serverVersion = g_cheatVersion;
    
    bool needsDownload = !fs::exists(exePath) || localVersion != serverVersion;
    
    if (needsDownload) {
        g_isDownloading = true;
        g_downloadProgress = 0.0f;
        g_status = "Downloading...";
        
        std::thread([exePath, serverVersion]() {
            bool success = api::Client::Get().DownloadCheat("cs2", exePath, 
                [](const api::DownloadProgress& p) {
                    g_downloadProgress = p.percent;
                });
            
            g_isDownloading = false;
            
            if (success) {
                SaveVersion(serverVersion);
                
                // Also get offsets
                std::string offsets = api::Client::Get().GetOffsets("cs2");
                if (!offsets.empty()) {
                    std::ofstream file("offsets.json");
                    file << offsets;
                    file.close();
                }
                
                g_status = "Downloaded! Click Launch again.";
            } else {
                g_status = "Download failed!";
            }
        }).detach();
        
        return;
    }
    
    // Launch the cheat
    g_status = "Launching...";
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (CreateProcessA(exePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        g_status = "Running! Enjoy :)";
        
        // Close launcher after delay
        std::thread([]() {
            Sleep(2000);
            PostQuitMessage(0);
        }).detach();
    } else {
        g_status = "Failed to launch! Run as admin?";
    }
}

// ============================================
// UI Drawing
// ============================================
void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 6.0f;
    
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(10, 10);
    
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = colors::bg;
    c[ImGuiCol_ChildBg] = colors::card;
    c[ImGuiCol_PopupBg] = colors::card;
    c[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.25f, 0.5f);
    
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
    
    c[ImGuiCol_Button] = ImVec4(colors::accent.x, colors::accent.y, colors::accent.z, 0.8f);
    c[ImGuiCol_ButtonHovered] = colors::accentHover;
    c[ImGuiCol_ButtonActive] = colors::accentDark;
    
    c[ImGuiCol_Header] = colors::accent;
    c[ImGuiCol_HeaderHovered] = colors::accentHover;
    c[ImGuiCol_HeaderActive] = colors::accentDark;
    
    c[ImGuiCol_Text] = colors::text;
    c[ImGuiCol_TextDisabled] = colors::textMuted;
    
    c[ImGuiCol_CheckMark] = colors::accent;
    c[ImGuiCol_SliderGrab] = colors::accent;
    c[ImGuiCol_SliderGrabActive] = colors::accentHover;
}

void DrawLoginScreen() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    
    // Animated gradient line at top
    float lineY = winPos.y + 10;
    for (int i = 0; i < (int)winSize.x; i++) {
        float t = (float)i / winSize.x;
        float wave = sinf(t * 6.28f + g_animTime * 2.0f) * 0.5f + 0.5f;
        int r = (int)(147 * (1.0f - wave) + 100 * wave);
        int g = (int)(112 * (1.0f - wave) + 149 * wave);
        int b = (int)(219 * (1.0f - wave) + 237 * wave);
        draw->AddLine(ImVec2(winPos.x + i, lineY), ImVec2(winPos.x + i, lineY + 3), IM_COL32(r, g, b, 255));
    }
    
    ImGui::Dummy(ImVec2(0, 30));
    
    // Logo
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    float logoWidth = ImGui::CalcTextSize("CS-LEGIT").x * 1.8f;
    ImGui::SetCursorPosX((winSize.x - logoWidth) / 2);
    ImGui::SetWindowFontScale(1.8f);
    ImGui::TextColored(colors::accent, "CS-LEGIT");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    
    ImGui::SetCursorPosX((winSize.x - ImGui::CalcTextSize("External ESP for CS2").x) / 2);
    ImGui::TextColored(colors::textMuted, "External ESP for CS2");
    
    ImGui::Dummy(ImVec2(0, 30));
    
    // Form
    float inputWidth = 280;
    float centerX = (winSize.x - inputWidth) / 2;
    
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::textMuted, "Username");
    ImGui::SetCursorPosX(centerX);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##user", g_username, sizeof(g_username));
    
    ImGui::Dummy(ImVec2(0, 5));
    
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::textMuted, "Password");
    ImGui::SetCursorPosX(centerX);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##pass", g_password, sizeof(g_password), ImGuiInputTextFlags_Password);
    
    ImGui::Dummy(ImVec2(0, 15));
    
    // Error
    if (!g_error.empty()) {
        ImGui::SetCursorPosX(centerX);
        ImGui::TextColored(colors::danger, "%s", g_error.c_str());
        ImGui::Dummy(ImVec2(0, 5));
    }
    
    // Buttons
    ImGui::SetCursorPosX(centerX);
    
    if (g_isLoading) {
        ImGui::Button("Loading...", ImVec2(inputWidth, 45));
    } else {
        if (ImGui::Button("LOGIN", ImVec2(inputWidth, 45))) {
            DoLogin();
        }
    }
    
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::textDim, "Don't have an account?");
    
    ImGui::SetCursorPosX(centerX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
    if (ImGui::Button("Register with License Key", ImVec2(inputWidth, 35))) {
        g_screen = Screen::Register;
        g_error = "";
    }
    ImGui::PopStyleColor(2);
    
    // Version
    ImGui::SetCursorPos(ImVec2(10, winSize.y - 25));
    ImGui::TextColored(colors::textDim, "v%s", LAUNCHER_VERSION);
}

void DrawRegisterScreen() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    
    // Same animated line
    float lineY = winPos.y + 10;
    for (int i = 0; i < (int)winSize.x; i++) {
        float t = (float)i / winSize.x;
        float wave = sinf(t * 6.28f + g_animTime * 2.0f) * 0.5f + 0.5f;
        int r = (int)(147 * (1.0f - wave) + 100 * wave);
        int g = (int)(112 * (1.0f - wave) + 149 * wave);
        int b = (int)(219 * (1.0f - wave) + 237 * wave);
        draw->AddLine(ImVec2(winPos.x + i, lineY), ImVec2(winPos.x + i, lineY + 3), IM_COL32(r, g, b, 255));
    }
    
    ImGui::Dummy(ImVec2(0, 20));
    
    // Title
    float titleWidth = ImGui::CalcTextSize("REGISTER").x * 1.5f;
    ImGui::SetCursorPosX((winSize.x - titleWidth) / 2);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(colors::accent, "REGISTER");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::Dummy(ImVec2(0, 20));
    
    float inputWidth = 280;
    float centerX = (winSize.x - inputWidth) / 2;
    
    // License Key (first!)
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::warning, "License Key");
    ImGui::SetCursorPosX(centerX);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##license", g_licenseKey, sizeof(g_licenseKey));
    
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::textMuted, "Username");
    ImGui::SetCursorPosX(centerX);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##user2", g_username, sizeof(g_username));
    
    ImGui::Dummy(ImVec2(0, 5));
    
    ImGui::SetCursorPosX(centerX);
    ImGui::TextColored(colors::textMuted, "Password");
    ImGui::SetCursorPosX(centerX);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##pass2", g_password, sizeof(g_password), ImGuiInputTextFlags_Password);
    
    ImGui::Dummy(ImVec2(0, 10));
    
    // Error
    if (!g_error.empty()) {
        ImGui::SetCursorPosX(centerX);
        ImGui::TextColored(colors::danger, "%s", g_error.c_str());
        ImGui::Dummy(ImVec2(0, 5));
    }
    
    // Buttons
    ImGui::SetCursorPosX(centerX);
    if (g_isLoading) {
        ImGui::Button("Loading...", ImVec2(inputWidth, 45));
    } else {
        if (ImGui::Button("REGISTER", ImVec2(inputWidth, 45))) {
            DoRegister();
        }
    }
    
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::SetCursorPosX(centerX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
    if (ImGui::Button("< Back to Login", ImVec2(inputWidth, 35))) {
        g_screen = Screen::Login;
        g_error = "";
    }
    ImGui::PopStyleColor(2);
}

void DrawMainScreen() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    
    // Header gradient line
    float lineY = winPos.y + 10;
    for (int i = 0; i < (int)winSize.x; i++) {
        float t = (float)i / winSize.x;
        float wave = sinf(t * 6.28f + g_animTime * 2.0f) * 0.5f + 0.5f;
        int r = (int)(147 * (1.0f - wave) + 100 * wave);
        int g = (int)(112 * (1.0f - wave) + 149 * wave);
        int b = (int)(219 * (1.0f - wave) + 237 * wave);
        draw->AddLine(ImVec2(winPos.x + i, lineY), ImVec2(winPos.x + i, lineY + 3), IM_COL32(r, g, b, 255));
    }
    
    ImGui::Dummy(ImVec2(0, 20));
    
    // Header with user info
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextColored(colors::accent, "CS-LEGIT");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::SameLine(winSize.x - 100);
    if (ImGui::SmallButton("Logout")) {
        Logout();
        return;
    }
    
    ImGui::Dummy(ImVec2(0, 5));
    ImGui::TextColored(colors::textMuted, "Welcome, %s", g_userDisplay.c_str());
    
    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 15));
    
    // Game Card
    float cardWidth = winSize.x - 40;
    float cardHeight = 120;
    float cardX = winPos.x + 20;
    float cardY = ImGui::GetCursorScreenPos().y;
    
    // Card background with glow
    for (int i = 4; i >= 1; i--) {
        draw->AddRectFilled(
            ImVec2(cardX - i * 2, cardY - i * 2),
            ImVec2(cardX + cardWidth + i * 2, cardY + cardHeight + i * 2),
            IM_COL32(147, 112, 219, 5 * i), 15.0f + i * 2);
    }
    
    draw->AddRectFilled(ImVec2(cardX, cardY), ImVec2(cardX + cardWidth, cardY + cardHeight),
                        IM_COL32(30, 28, 45, 255), 15.0f);
    draw->AddRect(ImVec2(cardX, cardY), ImVec2(cardX + cardWidth, cardY + cardHeight),
                  IM_COL32(147, 112, 219, 100), 15.0f, 0, 2.0f);
    
    // Game info inside card
    ImGui::SetCursorScreenPos(ImVec2(cardX + 20, cardY + 15));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(colors::text, "Counter-Strike 2");
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::SetCursorScreenPos(ImVec2(cardX + 20, cardY + 45));
    ImGui::TextColored(colors::textMuted, "External ESP");
    
    ImGui::SetCursorScreenPos(ImVec2(cardX + 20, cardY + 70));
    ImGui::TextColored(colors::textDim, "Version: ");
    ImGui::SameLine();
    ImGui::TextColored(colors::accent, "%s", g_cheatVersion.c_str());
    
    // License status
    ImGui::SetCursorScreenPos(ImVec2(cardX + 20, cardY + 90));
    ImGui::TextColored(colors::textDim, "License: ");
    ImGui::SameLine();
    ImGui::TextColored(g_hasLicense ? colors::success : colors::danger, "%s", g_licenseStatus.c_str());
    
    // CS2 status indicator
    bool cs2Running = IsCS2Running();
    ImVec2 statusPos = ImVec2(cardX + cardWidth - 100, cardY + 20);
    draw->AddCircleFilled(statusPos, 8, cs2Running ? IM_COL32(50, 205, 50, 255) : IM_COL32(255, 100, 100, 255));
    ImGui::SetCursorScreenPos(ImVec2(statusPos.x + 15, statusPos.y - 7));
    ImGui::TextColored(cs2Running ? colors::success : colors::danger, cs2Running ? "CS2 Running" : "CS2 Not Found");
    
    ImGui::SetCursorScreenPos(ImVec2(20, cardY + cardHeight + 20));
    ImGui::Dummy(ImVec2(0, 0));
    
    ImGui::Dummy(ImVec2(0, 10));
    
    // Big Launch Button
    float btnWidth = cardWidth;
    float btnHeight = 60;
    float btnX = (winSize.x - btnWidth) / 2;
    
    ImGui::SetCursorPosX(btnX);
    
    bool canLaunch = g_hasLicense && !g_isDownloading && !g_isLoading;
    
    if (g_isDownloading) {
        // Progress bar
        ImVec2 barPos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(barPos, ImVec2(barPos.x + btnWidth, barPos.y + btnHeight),
                            IM_COL32(40, 38, 60, 255), 30.0f);
        
        float progress = g_downloadProgress / 100.0f;
        draw->AddRectFilled(barPos, ImVec2(barPos.x + btnWidth * progress, barPos.y + btnHeight),
                            IM_COL32(147, 112, 219, 255), 30.0f);
        
        char progressText[32];
        sprintf(progressText, "Downloading... %.0f%%", g_downloadProgress.load());
        ImVec2 textSize = ImGui::CalcTextSize(progressText);
        draw->AddText(ImVec2(barPos.x + (btnWidth - textSize.x) / 2, barPos.y + (btnHeight - textSize.y) / 2),
                      IM_COL32(255, 255, 255, 255), progressText);
        
        ImGui::Dummy(ImVec2(btnWidth, btnHeight));
    } else {
        // Animated glow
        if (canLaunch) {
            ImVec2 btnPos = ImGui::GetCursorScreenPos();
            float pulse = sinf(g_animTime * 3.0f) * 0.3f + 0.7f;
            for (int i = 5; i >= 1; i--) {
                draw->AddRectFilled(
                    ImVec2(btnPos.x - i * 3, btnPos.y - i * 3),
                    ImVec2(btnPos.x + btnWidth + i * 3, btnPos.y + btnHeight + i * 3),
                    IM_COL32((int)(147 * pulse), (int)(112 * pulse), (int)(219 * pulse), 10 * i),
                    30.0f + i * 3);
            }
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, canLaunch ? 
            ImVec4(0.45f, 0.30f, 0.70f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.40f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.20f, 0.60f, 1.0f));
        
        if (ImGui::Button(canLaunch ? "LAUNCH" : (g_hasLicense ? "LOADING..." : "NO LICENSE"), 
                         ImVec2(btnWidth, btnHeight))) {
            if (canLaunch) LaunchCheat();
        }
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
    
    // Status message
    if (!g_status.empty()) {
        ImGui::Dummy(ImVec2(0, 15));
        float statusWidth = ImGui::CalcTextSize(g_status.c_str()).x;
        ImGui::SetCursorPosX((winSize.x - statusWidth) / 2);
        
        ImVec4 statusColor = colors::textMuted;
        if (g_status.find("Running") != std::string::npos || g_status.find("Enjoy") != std::string::npos)
            statusColor = colors::success;
        else if (g_status.find("failed") != std::string::npos || g_status.find("No license") != std::string::npos)
            statusColor = colors::danger;
        else if (g_status.find("Start CS2") != std::string::npos)
            statusColor = colors::warning;
            
        ImGui::TextColored(statusColor, "%s", g_status.c_str());
    }
    
    // Footer
    ImGui::SetCursorPos(ImVec2(10, winSize.y - 25));
    ImGui::TextColored(colors::textDim, "v%s", LAUNCHER_VERSION);
}

// ============================================
// Main
// ============================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"CSLegitLauncher";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
    
    // Create window
    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"CS-LEGIT Launcher",
        WS_POPUP,
        100, 100, 400, 550,
        NULL, NULL, hInstance, NULL);
    
    // Round corners (Windows 11)
    DWORD preference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
    
    // Create D3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;
    
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    SetupStyle();
    
    // Try auto-login
    TryAutoLogin();
    
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        // Update animation
        g_animTime += io.DeltaTime;
        
        // Start frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(400, 550));
        ImGui::Begin("##main", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
        switch (g_screen) {
            case Screen::Login: DrawLoginScreen(); break;
            case Screen::Register: DrawRegisterScreen(); break;
            case Screen::Main: DrawMainScreen(); break;
        }
        
        ImGui::End();
        
        // Render
        ImGui::Render();
        const float clear_color[4] = { 0.06f, 0.06f, 0.08f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        g_pSwapChain->Present(1, 0);
    }
    
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    
    return 0;
}

// ============================================
// D3D11 Setup
// ============================================
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
        &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;
    
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN:
        // Allow dragging window
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
        return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
