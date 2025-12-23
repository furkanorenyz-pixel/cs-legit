/*
 * EXTERNAL ESP - Premium Launcher v2.0.0
 * With Server Authentication & Auto-Update
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>
#include <fstream>

#pragma comment(lib, "dwmapi.lib")

// DWM Window Corner Preference (Windows 11)
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "xorstr.hpp"
#include "api.hpp"

namespace fs = std::filesystem;

// ============================================
// App State
// ============================================
enum class AppScreen {
    Login,
    Main
};

AppScreen g_currentScreen = AppScreen::Login;
char g_username[64] = "";
char g_password[64] = "";
std::string g_loginError = "";
bool g_isLoggingIn = false;
float g_downloadProgress = 0.0f;
bool g_isDownloading = false;

// ============================================
// Premium Color Palette
// ============================================
namespace colors {
    // Main gradient (Purple to Blue)
    constexpr ImU32 gradientStart = IM_COL32(138, 43, 226, 255);  // BlueViolet
    constexpr ImU32 gradientEnd = IM_COL32(30, 144, 255, 255);    // DodgerBlue
    
    // Accent colors
    constexpr ImU32 accent = IM_COL32(147, 112, 219, 255);        // MediumPurple
    constexpr ImU32 accentHover = IM_COL32(186, 85, 211, 255);    // MediumOrchid
    constexpr ImU32 accentActive = IM_COL32(218, 112, 214, 255);  // Orchid
    
    // Status colors
    constexpr ImU32 success = IM_COL32(50, 205, 50, 255);         // LimeGreen
    constexpr ImU32 warning = IM_COL32(255, 165, 0, 255);         // Orange
    constexpr ImU32 danger = IM_COL32(255, 69, 0, 255);           // OrangeRed
    
    // Background
    constexpr ImU32 bgDark = IM_COL32(13, 13, 18, 255);           // Almost black
    constexpr ImU32 bgCard = IM_COL32(22, 22, 30, 255);           // Card bg
    constexpr ImU32 bgCardHover = IM_COL32(32, 32, 42, 255);      // Card hover
    
    // Text
    constexpr ImU32 textPrimary = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 textSecondary = IM_COL32(160, 160, 180, 255);
    constexpr ImU32 textMuted = IM_COL32(100, 100, 120, 255);
}

// ============================================
// Globals
// ============================================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// State
std::string g_statusMsg = "Ready to launch";
ImU32 g_statusColor = colors::success;
float g_animTime = 0.0f;
bool g_isLaunching = false;

// ============================================
// Helpers
// ============================================
std::string RandomString(size_t length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string str(length, 0);
    for (size_t i = 0; i < length; ++i) str[i] = charset[dist(rng)];
    return str;
}

std::string GetCS2Path() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, XString("SOFTWARE\\WOW6432Node\\Valve\\Steam"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char path[MAX_PATH];
        DWORD len = MAX_PATH;
        if (RegQueryValueExA(hKey, XString("InstallPath"), nullptr, nullptr, (LPBYTE)path, &len) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            fs::path steamPath = path;
            fs::path cs2Path = steamPath / XString("steamapps/common/Counter-Strike Global Offensive/game/csgo/bin/win64/client.dll");
            if (fs::exists(cs2Path)) return cs2Path.string();
        }
        RegCloseKey(hKey);
    }
    return "";
}

bool CheckUpdates(const std::string& cheatFile) {
    std::string gameDll = GetCS2Path();
    if (gameDll.empty()) {
        g_statusMsg = XString("CS2 not found - proceed with caution");
        g_statusColor = colors::warning;
        return true;
    }

    try {
        if (!fs::exists(cheatFile)) {
            g_statusMsg = XString("Executable not found!");
            g_statusColor = colors::danger;
            return false;
        }

        auto gameTime = fs::last_write_time(gameDll);
        auto cheatTime = fs::last_write_time(cheatFile);

        if (gameTime > cheatTime) {
            g_statusMsg = XString("Game updated - wait for new build!");
            g_statusColor = colors::danger;
            return false;
        }
    } catch (...) {
        g_statusMsg = XString("Check failed - proceed anyway");
        g_statusColor = colors::warning;
        return true;
    }
    return true;
}

void Launch() {
    if (g_isLaunching || g_isDownloading) return;
    
    std::string gameId = "cs2";
    std::string target = XString("externa.exe");  // Always external
    
    // Check if we have license
    if (!api::Client::Get().HasLicense(gameId)) {
        g_statusMsg = XString("No license! Contact admin.");
        g_statusColor = colors::danger;
        return;
    }
    
    // Check if file exists or needs update
    bool needsDownload = !fs::exists(target);
    
    if (!needsDownload) {
        // Check for updates
        std::string currentVersion = "1.0.0"; // TODO: read from version file
        needsDownload = api::Client::Get().CheckUpdate(gameId, currentVersion);
    }
    
    if (needsDownload) {
        g_isDownloading = true;
        g_downloadProgress = 0.0f;
        g_statusMsg = XString("Downloading...");
        g_statusColor = colors::accent;
        
        std::thread([target, gameId]() {
            bool success = api::Client::Get().DownloadCheat(gameId, target, 
                [](const api::DownloadProgress& p) {
                    g_downloadProgress = p.percent;
                });
            
            g_isDownloading = false;
            
            if (success) {
                g_statusMsg = XString("Download complete! Click Launch again.");
                g_statusColor = colors::success;
                
                // Also update offsets
                std::string offsets = api::Client::Get().GetOffsets(gameId);
                if (!offsets.empty()) {
                    std::ofstream file("offsets.json");
                    file << offsets;
                    file.close();
                }
            } else {
                g_statusMsg = XString("Download failed!");
                g_statusColor = colors::danger;
            }
        }).detach();
        
        return;
    }

    g_isLaunching = true;
    g_statusMsg = XString("Launching CS2 External...");
    g_statusColor = colors::accent;
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(target.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        g_statusMsg = XString("CS2 External started!");
        g_statusColor = colors::success;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        std::thread([]() {
            Sleep(2000);
            exit(0);
        }).detach();
    } else {
        g_statusMsg = XString("Launch failed - run as admin?");
        g_statusColor = colors::danger;
        g_isLaunching = false;
    }
}

// ============================================
// Login Functions
// ============================================
void DoLogin() {
    if (g_isLoggingIn) return;
    if (strlen(g_username) == 0 || strlen(g_password) == 0) {
        g_loginError = "Enter username and password";
        return;
    }
    
    g_isLoggingIn = true;
    g_loginError = "";
    
    std::thread([]() {
        auto result = api::Client::Get().Login(g_username, g_password);
        
        g_isLoggingIn = false;
        
        if (result.success) {
            g_currentScreen = AppScreen::Main;
            // Load games
            api::Client::Get().GetGames();
        } else {
            g_loginError = result.error;
        }
    }).detach();
}

void TryAutoLogin() {
    if (api::Client::Get().LoadSession()) {
        g_currentScreen = AppScreen::Main;
        api::Client::Get().GetGames();
    }
}

// ============================================
// Premium UI Drawing
// ============================================
namespace ui {
    // Draw gradient background
    void DrawGradientBackground(ImDrawList* draw, ImVec2 pos, ImVec2 size, float radius = 20.0f) {
        // Base dark background with rounded corners
        ImU32 bgColor = IM_COL32(18, 16, 28, 255);
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, radius);
        
        // Subtle animated color overlay (top area)
        float shift = sinf(g_animTime * 0.5f) * 0.3f + 0.7f;
        ImU32 overlayTop = IM_COL32((int)(30 * shift), (int)(20 * shift), (int)(50 * shift), 100);
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y * 0.4f), overlayTop, radius);
    }
    
    // Draw glowing line
    void DrawGlowLine(ImDrawList* draw, ImVec2 start, ImVec2 end, ImU32 color, float thickness = 2.0f) {
        // Glow layers
        for (int i = 3; i >= 0; i--) {
            float alpha = 0.1f + (3 - i) * 0.2f;
            ImU32 glowColor = (color & 0x00FFFFFF) | ((int)(alpha * 255) << 24);
            draw->AddLine(start, end, glowColor, thickness + i * 2);
        }
        draw->AddLine(start, end, color, thickness);
    }
    
    // Draw animated gradient header line
    void DrawHeaderLine(ImDrawList* draw, ImVec2 pos, float width) {
        float animOffset = fmodf(g_animTime * 100.0f, width * 2);
        
        for (int i = 0; i < (int)width; i++) {
            float t = (float)i / width;
            float wave = sinf(t * 3.14159f * 2 + g_animTime * 3) * 0.5f + 0.5f;
            
            int r = (int)(138 + wave * 80);
            int g = (int)(43 + wave * 60);
            int b = (int)(226 - wave * 30);
            
            ImU32 col = IM_COL32(r, g, b, 255);
            draw->AddLine(ImVec2(pos.x + i, pos.y), ImVec2(pos.x + i, pos.y + 3), col);
        }
    }
    
    // Draw clean minimalist mode card with large radius
    bool DrawModeCard(ImDrawList* draw, const char* title, const char* subtitle, bool isExternal,
                      ImVec2 pos, ImVec2 size, bool selected, bool hovered) {
        
        // Large radius for smooth corners (like the button)
        float radius = 20.0f;
        
        // Card background colors
        ImU32 bgColor = selected ? IM_COL32(55, 35, 90, 255) : 
                        hovered ? IM_COL32(35, 32, 50, 255) : IM_COL32(22, 22, 32, 255);
        
        // Glow effect when selected
        if (selected) {
            for (int i = 5; i >= 1; i--) {
                ImU32 glowColor = IM_COL32(147, 112, 219, 10 + i * 10);
                draw->AddRectFilled(
                    ImVec2(pos.x - i * 3, pos.y - i * 3),
                    ImVec2(pos.x + size.x + i * 3, pos.y + size.y + i * 3),
                    glowColor, radius + i * 3);
            }
        }
        
        // Main card background
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, radius);
        
        // Border
        ImU32 borderColor = selected ? colors::accent : IM_COL32(60, 55, 80, 255);
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, radius, 0, selected ? 2.5f : 1.5f);
        
        
        // Title - larger and centered
        ImGui::SetWindowFontScale(1.3f);
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        float titleY = pos.y + (size.y - 45) / 2;
        ImGui::SetCursorScreenPos(ImVec2(pos.x + (size.x - titleSize.x) / 2, titleY));
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.85f, 1.0f));
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
        
        // Decorative line under title
        float lineY = titleY + titleSize.y + 8;
        float lineWidth = 40;
        ImU32 lineColor = selected ? colors::accent : IM_COL32(80, 70, 100, 255);
        draw->AddLine(ImVec2(pos.x + (size.x - lineWidth) / 2, lineY), 
                     ImVec2(pos.x + (size.x + lineWidth) / 2, lineY), lineColor, 2.0f);
        
        // Subtitle
        ImVec2 subSize = ImGui::CalcTextSize(subtitle);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + (size.x - subSize.x) / 2, lineY + 12));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.65f, 1.0f));
        ImGui::Text("%s", subtitle);
        ImGui::PopStyleColor();
        
        // Selected indicator - minimal dot
        if (selected) {
            float cx = pos.x + size.x - 18;
            float cy = pos.y + 18;
            draw->AddCircleFilled(ImVec2(cx, cy), 8, colors::success);
            // Checkmark
            draw->AddLine(ImVec2(cx - 4, cy), ImVec2(cx - 1, cy + 3), colors::textPrimary, 2.0f);
            draw->AddLine(ImVec2(cx - 1, cy + 3), ImVec2(cx + 4, cy - 3), colors::textPrimary, 2.0f);
        }
        
        return hovered;
    }
    
    // Draw pill-shaped button with perfect rounding
    bool DrawPremiumButton(ImDrawList* draw, const char* label, ImVec2 pos, ImVec2 size, bool disabled = false) {
        ImGui::SetCursorScreenPos(pos);
        
        ImVec2 mousePos = ImGui::GetMousePos();
        bool hovered = mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
                       mousePos.y >= pos.y && mousePos.y <= pos.y + size.y;
        bool clicked = hovered && ImGui::IsMouseClicked(0);
        
        if (disabled) {
            hovered = false;
            clicked = false;
        }
        
        // Pill radius = half of height for perfect rounding
        float radius = size.y / 2.0f;
        
        // Animated pulse
        float pulse = sinf(g_animTime * 2.5f) * 0.15f + 0.85f;
        
        // Colors
        ImU32 bgColor, borderColor;
        if (disabled) {
            bgColor = IM_COL32(45, 45, 55, 255);
            borderColor = IM_COL32(60, 60, 70, 255);
        } else if (hovered) {
            bgColor = IM_COL32(130, 80, 200, 255);
            borderColor = IM_COL32(180, 140, 255, 255);
        } else {
            int intensity = (int)(255 * pulse);
            bgColor = IM_COL32(90 * pulse, 50 * pulse, 160 * pulse, 255);
            borderColor = IM_COL32(147 * pulse, 112 * pulse, 219 * pulse, 200);
        }
        
        // Glow effect (outer)
        if (!disabled) {
            for (int i = 5; i >= 1; i--) {
                ImU32 glowColor = IM_COL32(147, 112, 219, (int)(8 * pulse * i));
                draw->AddRectFilled(
                    ImVec2(pos.x - i * 3, pos.y - i * 3),
                    ImVec2(pos.x + size.x + i * 3, pos.y + size.y + i * 3),
                    glowColor, radius + i * 3);
            }
        }
        
        // Main pill background
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, radius);
        
        // Border
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, radius, 0, 2.0f);
        
        // Inner highlight (top edge)
        if (!disabled) {
            draw->AddLine(
                ImVec2(pos.x + radius, pos.y + 2),
                ImVec2(pos.x + size.x - radius, pos.y + 2),
                IM_COL32(255, 255, 255, 40), 1.0f);
        }
        
        // Animated shine sweep
        if (!disabled) {
            float shineT = fmodf(g_animTime * 0.4f, 2.0f);
            if (shineT < 1.0f) {
                float shineX = pos.x + size.x * shineT;
                // Clip shine to button bounds
                float clipLeft = fmaxf(shineX - 40, pos.x + 10);
                float clipRight = fminf(shineX + 40, pos.x + size.x - 10);
                if (clipRight > clipLeft) {
                    draw->AddRectFilledMultiColor(
                        ImVec2(clipLeft, pos.y + 5),
                        ImVec2(clipRight, pos.y + size.y - 5),
                        IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 25),
                        IM_COL32(255, 255, 255, 25), IM_COL32(255, 255, 255, 0));
                }
            }
        }
        
        // Text with shadow
        ImVec2 textSize = ImGui::CalcTextSize(label);
        float textX = pos.x + (size.x - textSize.x) / 2;
        float textY = pos.y + (size.y - textSize.y) / 2;
        
        // Shadow
        if (!disabled) {
            draw->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 100), label);
        }
        
        // Main text
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, disabled ? ImVec4(0.4f, 0.4f, 0.45f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("%s", label);
        ImGui::PopStyleColor();
        
        return clicked && !disabled;
    }
    
    // Draw status bar
    // Draw simple status text (no background bar)
    void DrawStatusText(ImDrawList* draw, ImVec2 pos, const char* status, ImU32 color) {
        // Animated dot
        float pulse = sinf(g_animTime * 4) * 0.3f + 0.7f;
        ImU32 dotColor = (color & 0x00FFFFFF) | ((int)(255 * pulse) << 24);
        draw->AddCircleFilled(ImVec2(pos.x, pos.y + 7), 5, dotColor);
        
        // Text
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 15, pos.y));
        ImU32 textColor = (color & 0x00FFFFFF) | (200 << 24);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(textColor));
        ImGui::Text("%s", status);
        ImGui::PopStyleColor();
    }
    
    // Draw version badge
    void DrawVersionBadge(ImDrawList* draw, ImVec2 pos, const char* version) {
        ImVec2 textSize = ImGui::CalcTextSize(version);
        ImVec2 badgeSize = ImVec2(textSize.x + 16, textSize.y + 8);
        
        // Badge background
        draw->AddRectFilled(pos, ImVec2(pos.x + badgeSize.x, pos.y + badgeSize.y),
            IM_COL32(50, 50, 70, 200), 4.0f);
        draw->AddRect(pos, ImVec2(pos.x + badgeSize.x, pos.y + badgeSize.y),
            IM_COL32(80, 80, 100, 255), 4.0f);
        
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y + 4));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.8f, 1.0f));
        ImGui::Text("%s", version);
        ImGui::PopStyleColor();
    }
}

// ============================================
// Premium Theme Setup
// ============================================
void SetupPremiumTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Rounding
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 8.0f;
    
    // Padding
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    
    // Colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Transparent for custom bg
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
}

// ============================================
// DirectX Boilerplate
// ============================================
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================
// Main
// ============================================
// Draw Login Screen
void DrawLoginScreen(ImDrawList* draw, ImVec2 winPos, ImVec2 winSize) {
    // Title
    const char* titleText = XString("LOGIN");
    ImGui::SetWindowFontScale(2.0f);
    ImVec2 titleSize = ImGui::CalcTextSize(titleText);
    ImVec2 titlePos = ImVec2(winPos.x + (winSize.x - titleSize.x) / 2, winPos.y + 60);
    
    ImGui::SetCursorScreenPos(titlePos);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.75f, 1.0f, 1.0f));
    ImGui::Text("%s", titleText);
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);
    
    // Username input
    float inputWidth = 280;
    float inputX = winPos.x + (winSize.x - inputWidth) / 2;
    float inputY = winPos.y + 130;
    
    ImGui::SetCursorScreenPos(ImVec2(inputX, inputY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 12));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.25f, 0.45f, 1.0f));
    ImGui::PushItemWidth(inputWidth);
    
    ImGui::InputTextWithHint("##username", "Username", g_username, sizeof(g_username));
    
    // Password input
    ImGui::SetCursorScreenPos(ImVec2(inputX, inputY + 55));
    ImGui::InputTextWithHint("##password", "Password", g_password, sizeof(g_password), 
        ImGuiInputTextFlags_Password);
    
    ImGui::PopItemWidth();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    
    // Error message
    if (!g_loginError.empty()) {
        ImVec2 errorSize = ImGui::CalcTextSize(g_loginError.c_str());
        ImGui::SetCursorScreenPos(ImVec2(winPos.x + (winSize.x - errorSize.x) / 2, inputY + 115));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("%s", g_loginError.c_str());
        ImGui::PopStyleColor();
    }
    
    // Login button
    float buttonY = inputY + 145;
    if (ui::DrawPremiumButton(draw, 
        g_isLoggingIn ? XString("LOGGING IN...") : XString("LOGIN"),
        ImVec2(inputX, buttonY), ImVec2(inputWidth, 48), g_isLoggingIn)) {
        DoLogin();
    }
    
    // Enter key to login
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !g_isLoggingIn) {
        DoLogin();
    }
    
    // Server status
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + 20, winPos.y + winSize.y - 30));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.5f, 1.0f));
    ImGui::Text("Server: %s", "138.124.0.8");
    ImGui::PopStyleColor();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Try auto login from saved session
    TryAutoLogin();
    
    // Random window title for stealth
    std::string title = RandomString(12);
    
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, 
        L"PremiumLauncher", nullptr };
    RegisterClassExW(&wc);
    
    // Window size - compact and clean
    const int WIDTH = 420;
    const int HEIGHT = 340;
    
    // Center on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - WIDTH) / 2;
    int posY = (screenH - HEIGHT) / 2;
    
    HWND hwnd = CreateWindowW(wc.lpszClassName, 
        std::wstring(title.begin(), title.end()).c_str(), 
        WS_POPUP, // Borderless
        posX, posY, WIDTH, HEIGHT, 
        nullptr, nullptr, wc.hInstance, nullptr);

    // Enable rounded corners
    // Method 1: Windows 11 DWM API
    DWM_WINDOW_CORNER_PREFERENCE preference = (DWM_WINDOW_CORNER_PREFERENCE)DWMWCP_ROUND;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    
    // Method 2: Fallback for Windows 10 and below - create rounded region
    if (FAILED(hr)) {
        HRGN hRgn = CreateRoundRectRgn(0, 0, WIDTH + 1, HEIGHT + 1, 20, 20);
        SetWindowRgn(hwnd, hRgn, TRUE);
    }

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
    io.IniFilename = nullptr;
    
    SetupPremiumTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Animation timer
    auto startTime = std::chrono::high_resolution_clock::now();
    
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Update animation time
        auto now = std::chrono::high_resolution_clock::now();
        g_animTime = std::chrono::duration<float>(now - startTime).count();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main Window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
        ImGui::Begin("##main", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        
        // ========== BACKGROUND ==========
        ui::DrawGradientBackground(draw, winPos, winSize);
        
        // Window border with glow - large radius for smooth corners
        float windowRadius = 20.0f;
        for (int i = 3; i >= 0; i--) {
            draw->AddRect(
                ImVec2(winPos.x + i, winPos.y + i),
                ImVec2(winPos.x + winSize.x - i, winPos.y + winSize.y - i),
                IM_COL32(147, 112, 219, 20 + i * 15), windowRadius);
        }
        draw->AddRect(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
            IM_COL32(80, 60, 120, 255), windowRadius);
        
        // ========== HEADER ==========
        // Animated gradient line
        ui::DrawHeaderLine(draw, ImVec2(winPos.x, winPos.y), winSize.x);
        
        // Close button
        ImVec2 closePos = ImVec2(winPos.x + winSize.x - 35, winPos.y + 10);
        bool closeHovered = ImGui::IsMouseHoveringRect(closePos, ImVec2(closePos.x + 25, closePos.y + 25));
        draw->AddCircleFilled(ImVec2(closePos.x + 12, closePos.y + 12), 12, 
            closeHovered ? IM_COL32(255, 80, 80, 255) : IM_COL32(60, 60, 80, 255));
        draw->AddText(ImVec2(closePos.x + 7, closePos.y + 4), colors::textPrimary, "X");
        if (closeHovered && ImGui::IsMouseClicked(0)) done = true;
        
        // Minimize button
        ImVec2 minPos = ImVec2(winPos.x + winSize.x - 65, winPos.y + 10);
        bool minHovered = ImGui::IsMouseHoveringRect(minPos, ImVec2(minPos.x + 25, minPos.y + 25));
        draw->AddCircleFilled(ImVec2(minPos.x + 12, minPos.y + 12), 12, 
            minHovered ? IM_COL32(255, 180, 0, 255) : IM_COL32(60, 60, 80, 255));
        draw->AddText(ImVec2(minPos.x + 8, minPos.y + 2), colors::textPrimary, "-");
        if (minHovered && ImGui::IsMouseClicked(0)) ShowWindow(hwnd, SW_MINIMIZE);
        
        // Version badge
        ui::DrawVersionBadge(draw, ImVec2(winPos.x + 15, winPos.y + 12), XString("v2.0.0"));
        
        // ========== SCREEN SWITCH ==========
        if (g_currentScreen == AppScreen::Login) {
            DrawLoginScreen(draw, winPos, winSize);
            ImGui::End();
            
            // Render
            ImGui::Render();
            float clear_color[] = { 18.0f/255.0f, 16.0f/255.0f, 28.0f/255.0f, 1.0f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_pSwapChain->Present(1, 0);
            continue;
        }
        
        // Logout button (top left, after version badge)
        ImVec2 logoutPos = ImVec2(winPos.x + 85, winPos.y + 12);
        bool logoutHovered = ImGui::IsMouseHoveringRect(logoutPos, ImVec2(logoutPos.x + 50, logoutPos.y + 22));
        draw->AddRectFilled(logoutPos, ImVec2(logoutPos.x + 50, logoutPos.y + 22),
            logoutHovered ? IM_COL32(80, 60, 100, 255) : IM_COL32(50, 50, 70, 200), 4.0f);
        ImGui::SetCursorScreenPos(ImVec2(logoutPos.x + 8, logoutPos.y + 4));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.8f, 1.0f));
        ImGui::Text("Logout");
        ImGui::PopStyleColor();
        if (logoutHovered && ImGui::IsMouseClicked(0)) {
            api::Client::Get().Logout();
            g_currentScreen = AppScreen::Login;
            g_username[0] = 0;
            g_password[0] = 0;
        }
        
        // ========== TITLE ==========
        const char* titleText = XString("CS2 EXTERNAL");
        
        ImGui::SetWindowFontScale(2.0f);
        ImVec2 titleSize = ImGui::CalcTextSize(titleText);
        ImVec2 titlePos = ImVec2(winPos.x + (winSize.x - titleSize.x) / 2, winPos.y + 55);
        
        // Glow effect
        for (int i = 3; i >= 1; i--) {
            ImGui::SetCursorScreenPos(ImVec2(titlePos.x, titlePos.y));
            ImU32 glowCol = IM_COL32(147, 112, 219, 30 / i);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), 
                ImVec2(titlePos.x - i, titlePos.y - i), glowCol, titleText);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), 
                ImVec2(titlePos.x + i, titlePos.y + i), glowCol, titleText);
        }
        
        // Main title
        ImGui::SetCursorScreenPos(titlePos);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.75f, 1.0f, 1.0f));
        ImGui::Text("%s", titleText);
        ImGui::PopStyleColor();
        
        ImGui::SetWindowFontScale(1.0f);
        
        // Decorative line under title
        float lineY = titlePos.y + titleSize.y + 5;
        float lineWidth = 120;
        draw->AddLine(ImVec2(winPos.x + (winSize.x - lineWidth) / 2, lineY),
                     ImVec2(winPos.x + (winSize.x + lineWidth) / 2, lineY),
                     IM_COL32(147, 112, 219, 150), 2.0f);
        
        // ========== CS2 INFO CARD ==========
        float cardWidth = 320;
        float cardHeight = 100;
        float cardX = winPos.x + (winSize.x - cardWidth) / 2;
        float cardY = winPos.y + 115;
        
        // Card background
        draw->AddRectFilled(
            ImVec2(cardX, cardY),
            ImVec2(cardX + cardWidth, cardY + cardHeight),
            IM_COL32(30, 28, 45, 255), 15.0f);
        draw->AddRect(
            ImVec2(cardX, cardY),
            ImVec2(cardX + cardWidth, cardY + cardHeight),
            IM_COL32(80, 70, 120, 255), 15.0f, 0, 1.5f);
        
        // Game icon placeholder (CS2 text)
        ImGui::SetWindowFontScale(1.4f);
        ImGui::SetCursorScreenPos(ImVec2(cardX + 20, cardY + 20));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Text("CS2");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
        
        // Game name
        ImGui::SetCursorScreenPos(ImVec2(cardX + 75, cardY + 18));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("Counter-Strike 2");
        ImGui::PopStyleColor();
        
        // Mode info
        ImGui::SetCursorScreenPos(ImVec2(cardX + 75, cardY + 40));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
        ImGui::Text("External Overlay | ESP + Radar");
        ImGui::PopStyleColor();
        
        // License status
        bool hasLicense = api::Client::Get().HasLicense("cs2");
        ImGui::SetCursorScreenPos(ImVec2(cardX + 75, cardY + 62));
        if (hasLicense) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
            ImGui::Text("License: ACTIVE");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::Text("License: NONE");
        }
        ImGui::PopStyleColor();
        
        // Version badge on card
        std::string verText = "v" + api::Client::Get().GetLatestVersion("cs2");
        if (verText == "v") verText = "v1.0.0";
        ImVec2 verSize = ImGui::CalcTextSize(verText.c_str());
        draw->AddRectFilled(
            ImVec2(cardX + cardWidth - verSize.x - 25, cardY + 15),
            ImVec2(cardX + cardWidth - 10, cardY + 35),
            IM_COL32(80, 60, 120, 255), 8.0f);
        ImGui::SetCursorScreenPos(ImVec2(cardX + cardWidth - verSize.x - 18, cardY + 18));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
        ImGui::Text("%s", verText.c_str());
        ImGui::PopStyleColor();
        
        // ========== LAUNCH BUTTON ==========
        float buttonWidth = 280;
        float buttonHeight = 48;
        float buttonX = winPos.x + (winSize.x - buttonWidth) / 2;
        float buttonY = cardY + cardHeight + 25;
        
        // Determine button text based on state
        const char* buttonText = XString("LAUNCH CS2");
        bool buttonDisabled = g_isLaunching || g_isDownloading;
        
        if (g_isLaunching) {
            buttonText = XString("LAUNCHING...");
        } else if (g_isDownloading) {
            buttonText = XString("DOWNLOADING...");
        } else if (!api::Client::Get().HasLicense("cs2")) {
            buttonText = XString("NO LICENSE");
            buttonDisabled = true;
        }
        
        if (ui::DrawPremiumButton(draw, buttonText,
            ImVec2(buttonX, buttonY), ImVec2(buttonWidth, buttonHeight), buttonDisabled)) {
            Launch();
        }
        
        // ========== STATUS TEXT ==========
        float statusY = buttonY + buttonHeight + 20;
        
        // Download progress bar
        if (g_isDownloading) {
            float barWidth = 280;
            float barHeight = 6;
            float barX = winPos.x + (winSize.x - barWidth) / 2;
            
            // Background
            draw->AddRectFilled(
                ImVec2(barX, statusY),
                ImVec2(barX + barWidth, statusY + barHeight),
                IM_COL32(40, 40, 50, 255), 3.0f);
            
            // Progress
            float progressWidth = barWidth * (g_downloadProgress / 100.0f);
            draw->AddRectFilled(
                ImVec2(barX, statusY),
                ImVec2(barX + progressWidth, statusY + barHeight),
                colors::accent, 3.0f);
            
            // Percentage text
            char progText[32];
            snprintf(progText, sizeof(progText), "Downloading... %.0f%%", g_downloadProgress);
            ImVec2 textSize = ImGui::CalcTextSize(progText);
            ImGui::SetCursorScreenPos(ImVec2(winPos.x + (winSize.x - textSize.x) / 2, statusY + 12));
            ImGui::Text("%s", progText);
        } else {
            ui::DrawStatusText(draw, ImVec2(winPos.x + 30, statusY), g_statusMsg.c_str(), g_statusColor);
        }
        
        // User info (bottom right)
        if (api::Client::Get().IsLoggedIn()) {
            std::string userInfo = "User: " + api::Client::Get().GetUser().username;
            ImVec2 userSize = ImGui::CalcTextSize(userInfo.c_str());
            ImGui::SetCursorScreenPos(ImVec2(winPos.x + winSize.x - userSize.x - 20, winPos.y + winSize.y - 25));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
            ImGui::Text("%s", userInfo.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::End();

        // Render
        ImGui::Render();
        // Clear color matches background so corners blend
        float clear_color[] = { 18.0f/255.0f, 16.0f/255.0f, 28.0f/255.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// ============================================
// DirectX Implementation
// ============================================
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
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

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, 
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// For window dragging
static bool g_dragging = false;
static POINT g_dragStart;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_LBUTTONDOWN: {
        // Check if clicking on title area (for dragging)
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        if (pt.y < 45 && pt.x < 400) { // Title bar area
            g_dragging = true;
            g_dragStart = pt;
            SetCapture(hWnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (g_dragging) {
            g_dragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEMOVE:
        if (g_dragging) {
            POINT pt;
            GetCursorPos(&pt);
            RECT rect;
            GetWindowRect(hWnd, &rect);
            int newX = pt.x - g_dragStart.x;
            int newY = pt.y - g_dragStart.y;
            SetWindowPos(hWnd, nullptr, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
