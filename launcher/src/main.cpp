/*
 * CS-LEGIT Premium Launcher v3.0
 * Professional Gaming Software
 * 
 * Beautiful animated GUI with DirectX11 + ImGui
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>
#include <fstream>
#include <atomic>
#include <vector>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "wininet.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace fs = std::filesystem;

// ============================================
// Configuration
// ============================================
#define LAUNCHER_VERSION "3.0.0"
#define WINDOW_WIDTH 520
#define WINDOW_HEIGHT 680

// Server config - change these for your server
#define SERVER_HOST "138.124.0.8"
#define SERVER_PORT 80

// ============================================
// App State
// ============================================
enum class Screen { Splash, Login, Register, Main };

Screen g_currentScreen = Screen::Splash;
char g_username[64] = "";
char g_password[64] = "";
char g_licenseKey[64] = "";
std::string g_errorMsg = "";
std::string g_statusMsg = "";
std::string g_token = "";

std::atomic<bool> g_isLoading{false};
std::atomic<float> g_downloadProgress{0.0f};
std::atomic<bool> g_isDownloading{false};

// Animation timers
float g_animTimer = 0.0f;
float g_splashTimer = 0.0f;
float g_fadeAlpha = 0.0f;
float g_pulseValue = 0.0f;
float g_particleTime = 0.0f;

// User data
std::string g_displayName = "";
std::string g_subscriptionStatus = "";
std::string g_cheatVersion = "1.0.0";
bool g_hasValidLicense = false;

// Particles for background effect
struct Particle {
    float x, y;
    float speed;
    float size;
    float alpha;
};
std::vector<Particle> g_particles;

// ============================================
// Premium Color Scheme
// ============================================
namespace theme {
    // Main colors
    const ImVec4 background = ImVec4(0.04f, 0.04f, 0.06f, 1.0f);
    const ImVec4 backgroundAlt = ImVec4(0.06f, 0.06f, 0.09f, 1.0f);
    const ImVec4 surface = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    const ImVec4 surfaceHover = ImVec4(0.10f, 0.10f, 0.15f, 1.0f);
    const ImVec4 surfaceBorder = ImVec4(0.15f, 0.15f, 0.22f, 1.0f);
    
    // Accent - Premium Purple/Violet
    const ImVec4 accent = ImVec4(0.55f, 0.36f, 0.95f, 1.0f);
    const ImVec4 accentHover = ImVec4(0.65f, 0.46f, 1.0f, 1.0f);
    const ImVec4 accentGlow = ImVec4(0.55f, 0.36f, 0.95f, 0.3f);
    
    // Secondary - Cyan accent
    const ImVec4 secondary = ImVec4(0.0f, 0.85f, 0.95f, 1.0f);
    const ImVec4 secondaryDim = ImVec4(0.0f, 0.85f, 0.95f, 0.5f);
    
    // Status colors
    const ImVec4 success = ImVec4(0.15f, 0.85f, 0.45f, 1.0f);
    const ImVec4 warning = ImVec4(1.0f, 0.70f, 0.0f, 1.0f);
    const ImVec4 error = ImVec4(0.95f, 0.25f, 0.30f, 1.0f);
    
    // Text
    const ImVec4 text = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
    const ImVec4 textSecondary = ImVec4(0.60f, 0.60f, 0.68f, 1.0f);
    const ImVec4 textDim = ImVec4(0.40f, 0.40f, 0.48f, 1.0f);
    
    // Gradient colors (as ImU32)
    const ImU32 gradientStart = IM_COL32(140, 90, 245, 255);
    const ImU32 gradientEnd = IM_COL32(60, 180, 255, 255);
    const ImU32 glowPurple = IM_COL32(140, 90, 245, 80);
    const ImU32 glowCyan = IM_COL32(0, 220, 255, 60);
}

// ============================================
// DirectX Resources
// ============================================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hwnd = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================
// Utility Functions
// ============================================
float EaseOutCubic(float t) {
    return 1.0f - powf(1.0f - t, 3.0f);
}

float EaseInOutSine(float t) {
    return -(cosf(3.14159f * t) - 1.0f) / 2.0f;
}

std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string appPath = std::string(path) + "\\CS-Legit";
        CreateDirectoryA(appPath.c_str(), NULL);
        return appPath;
    }
    return ".";
}

void SaveSession(const std::string& token, const std::string& username) {
    std::ofstream file(GetAppDataPath() + "\\session.dat");
    if (file.is_open()) {
        file << token << "\n" << username;
        file.close();
    }
}

bool LoadSession() {
    std::ifstream file(GetAppDataPath() + "\\session.dat");
    if (file.is_open()) {
        std::getline(file, g_token);
        std::getline(file, g_displayName);
        file.close();
        return !g_token.empty();
    }
    return false;
}

void ClearSession() {
    g_token = "";
    g_displayName = "";
    DeleteFileA((GetAppDataPath() + "\\session.dat").c_str());
}

// ============================================
// Particle System
// ============================================
void InitParticles() {
    g_particles.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(0.0f, (float)WINDOW_WIDTH);
    std::uniform_real_distribution<float> distY(0.0f, (float)WINDOW_HEIGHT);
    std::uniform_real_distribution<float> distSpeed(10.0f, 40.0f);
    std::uniform_real_distribution<float> distSize(1.0f, 3.0f);
    std::uniform_real_distribution<float> distAlpha(0.1f, 0.4f);
    
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.x = distX(gen);
        p.y = distY(gen);
        p.speed = distSpeed(gen);
        p.size = distSize(gen);
        p.alpha = distAlpha(gen);
        g_particles.push_back(p);
    }
}

void UpdateParticles(float deltaTime) {
    for (auto& p : g_particles) {
        p.y -= p.speed * deltaTime;
        if (p.y < -10) {
            p.y = WINDOW_HEIGHT + 10;
            p.x = (float)(rand() % WINDOW_WIDTH);
        }
    }
}

void DrawParticles(ImDrawList* drawList) {
    for (const auto& p : g_particles) {
        ImU32 col = IM_COL32(140, 90, 245, (int)(p.alpha * 255));
        drawList->AddCircleFilled(ImVec2(p.x, p.y), p.size, col);
    }
}

// ============================================
// Custom UI Components
// ============================================
void DrawGradientRect(ImDrawList* drawList, ImVec2 pos, ImVec2 size, ImU32 colTop, ImU32 colBottom) {
    drawList->AddRectFilledMultiColor(
        pos, 
        ImVec2(pos.x + size.x, pos.y + size.y),
        colTop, colTop, colBottom, colBottom
    );
}

void DrawGlowRect(ImDrawList* drawList, ImVec2 pos, ImVec2 size, ImU32 color, float intensity = 1.0f) {
    for (int i = 0; i < 3; i++) {
        float expand = (float)(i + 1) * 4.0f;
        ImU32 glowCol = IM_COL32(
            (color >> 0) & 0xFF,
            (color >> 8) & 0xFF,
            (color >> 16) & 0xFF,
            (int)(20.0f * intensity / (i + 1))
        );
        drawList->AddRect(
            ImVec2(pos.x - expand, pos.y - expand),
            ImVec2(pos.x + size.x + expand, pos.y + size.y + expand),
            glowCol, 12.0f + expand, 0, 2.0f
        );
    }
}

bool AnimatedButton(const char* label, ImVec2 size, bool enabled = true) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    
    if (!enabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    } else {
        float pulse = (sinf(g_animTimer * 3.0f) + 1.0f) * 0.5f * 0.1f;
        ImVec4 btnColor = theme::accent;
        btnColor.x += pulse;
        btnColor.z += pulse;
        
        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::accentHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme::accent);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    }
    
    bool clicked = ImGui::Button(label, size);
    
    // Draw glow effect on hover
    if (enabled && ImGui::IsItemHovered()) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        DrawGlowRect(drawList, min, ImVec2(max.x - min.x, max.y - min.y), theme::gradientStart, 0.5f);
    }
    
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    
    return clicked && enabled;
}

void StyledInputText(const char* label, char* buf, size_t bufSize, bool isPassword = false) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 14));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme::surface);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme::surfaceHover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme::surfaceHover);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::surfaceBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    
    ImGuiInputTextFlags flags = isPassword ? ImGuiInputTextFlags_Password : 0;
    ImGui::InputText(label, buf, bufSize, flags);
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);
}

void DrawLogo(ImDrawList* drawList, ImVec2 center, float size) {
    // Animated logo with glow
    float pulse = (sinf(g_animTimer * 2.0f) + 1.0f) * 0.5f;
    float glowSize = size + 10.0f + pulse * 5.0f;
    
    // Outer glow
    for (int i = 0; i < 3; i++) {
        float s = glowSize + i * 8;
        ImU32 col = IM_COL32(140, 90, 245, 30 - i * 8);
        drawList->AddCircle(center, s, col, 32, 3.0f);
    }
    
    // Main circle with gradient
    drawList->AddCircleFilled(center, size, theme::gradientStart, 32);
    
    // Inner highlight
    drawList->AddCircleFilled(
        ImVec2(center.x - size * 0.2f, center.y - size * 0.2f),
        size * 0.3f,
        IM_COL32(255, 255, 255, 40),
        16
    );
    
    // CS text
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    const char* text = "CS";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    drawList->AddText(
        ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
        IM_COL32(255, 255, 255, 255),
        text
    );
    ImGui::PopFont();
}

void DrawFeatureCard(ImDrawList* drawList, ImVec2 pos, const char* icon, const char* title, const char* desc) {
    ImVec2 size(140, 90);
    
    // Card background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
        IM_COL32(20, 20, 30, 200), 10.0f);
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
        IM_COL32(60, 60, 80, 100), 10.0f, 0, 1.0f);
    
    // Icon
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 12, pos.y + 12));
    ImGui::TextColored(theme::accent, "%s", icon);
    
    // Title
    drawList->AddText(ImVec2(pos.x + 12, pos.y + 38), IM_COL32(255, 255, 255, 255), title);
    
    // Description
    drawList->AddText(ImVec2(pos.x + 12, pos.y + 58), IM_COL32(150, 150, 170, 255), desc);
}

// ============================================
// API Functions
// ============================================
std::string HttpPost(const std::string& path, const std::string& data) {
    HINTERNET hInternet = InternetOpenA("CS-Legit Launcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";
    
    HINTERNET hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }
    
    std::string headers = "Content-Type: application/json\r\n";
    if (!g_token.empty()) {
        headers += "Authorization: Bearer " + g_token + "\r\n";
    }
    
    if (!HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)data.c_str(), (DWORD)data.length())) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }
    
    std::string response;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return response;
}

std::string ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    
    size_t end = pos;
    bool inString = json[pos - 1] == '"';
    
    if (inString) {
        end = json.find('"', pos);
    } else {
        while (end < json.length() && json[end] != ',' && json[end] != '}') end++;
    }
    
    return json.substr(pos, end - pos);
}

void DoLogin() {
    if (strlen(g_username) == 0 || strlen(g_password) == 0) {
        g_errorMsg = "Please enter username and password";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    g_statusMsg = "Connecting...";
    
    std::thread([]() {
        std::string data = "{\"username\":\"" + std::string(g_username) + 
                          "\",\"password\":\"" + std::string(g_password) + "\"}";
        
        std::string response = HttpPost("/api/auth/login", data);
        
        if (response.empty()) {
            g_errorMsg = "Connection failed. Check your internet.";
            g_isLoading = false;
            return;
        }
        
        if (response.find("\"token\"") != std::string::npos) {
            g_token = ExtractJsonValue(response, "token");
            g_displayName = g_username;
            g_hasValidLicense = true;
            SaveSession(g_token, g_displayName);
            g_currentScreen = Screen::Main;
            g_fadeAlpha = 0.0f;
        } else {
            g_errorMsg = ExtractJsonValue(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Login failed";
        }
        
        g_isLoading = false;
        g_statusMsg = "";
    }).detach();
}

void DoRegister() {
    if (strlen(g_username) == 0 || strlen(g_password) == 0 || strlen(g_licenseKey) == 0) {
        g_errorMsg = "Please fill all fields";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    g_statusMsg = "Creating account...";
    
    std::thread([]() {
        std::string data = "{\"username\":\"" + std::string(g_username) + 
                          "\",\"password\":\"" + std::string(g_password) + 
                          "\",\"license_key\":\"" + std::string(g_licenseKey) + "\"}";
        
        std::string response = HttpPost("/api/auth/register", data);
        
        if (response.empty()) {
            g_errorMsg = "Connection failed";
            g_isLoading = false;
            return;
        }
        
        if (response.find("\"success\"") != std::string::npos) {
            g_statusMsg = "Account created! Please login.";
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
        } else {
            g_errorMsg = ExtractJsonValue(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Registration failed";
        }
        
        g_isLoading = false;
    }).detach();
}

void LaunchCheat() {
    g_isDownloading = true;
    g_downloadProgress = 0.0f;
    g_statusMsg = "Preparing...";
    
    std::thread([]() {
        // Simulate download progress
        for (int i = 0; i <= 100; i += 5) {
            g_downloadProgress = (float)i / 100.0f;
            g_statusMsg = "Downloading... " + std::to_string(i) + "%";
            Sleep(50);
        }
        
        g_statusMsg = "Launching...";
        Sleep(500);
        
        // Here would be actual download and launch code
        // For now just show success
        g_statusMsg = "Cheat is running!";
        g_isDownloading = false;
    }).detach();
}

// ============================================
// Screen Renderers
// ============================================
void RenderSplashScreen() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImVec2 center = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f);
    
    // Background with gradient
    DrawGradientRect(drawList, ImVec2(0, 0), windowSize, 
        IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    
    // Particles
    DrawParticles(drawList);
    
    // Animated logo
    float logoScale = EaseOutCubic(fminf(g_splashTimer / 0.5f, 1.0f));
    DrawLogo(drawList, ImVec2(center.x, center.y - 50), 50.0f * logoScale);
    
    // Title with fade
    float textAlpha = EaseOutCubic(fminf((g_splashTimer - 0.3f) / 0.5f, 1.0f));
    if (textAlpha > 0) {
        ImU32 textCol = IM_COL32(255, 255, 255, (int)(textAlpha * 255));
        
        const char* title = "CS-LEGIT";
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        drawList->AddText(ImVec2(center.x - titleSize.x * 0.5f, center.y + 30), textCol, title);
        
        const char* subtitle = "Premium Gaming Software";
        ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
        ImU32 subCol = IM_COL32(140, 90, 245, (int)(textAlpha * 200));
        drawList->AddText(ImVec2(center.x - subtitleSize.x * 0.5f, center.y + 55), subCol, subtitle);
    }
    
    // Loading bar
    float barWidth = 200.0f;
    float barHeight = 4.0f;
    float progress = fminf(g_splashTimer / 2.0f, 1.0f);
    
    ImVec2 barPos = ImVec2(center.x - barWidth * 0.5f, center.y + 100);
    drawList->AddRectFilled(barPos, ImVec2(barPos.x + barWidth, barPos.y + barHeight), 
        IM_COL32(40, 40, 60, 255), 2.0f);
    drawList->AddRectFilled(barPos, ImVec2(barPos.x + barWidth * progress, barPos.y + barHeight), 
        theme::gradientStart, 2.0f);
    
    // Transition to login after splash
    if (g_splashTimer > 2.5f) {
        if (LoadSession()) {
            g_currentScreen = Screen::Main;
        } else {
            g_currentScreen = Screen::Login;
        }
        g_fadeAlpha = 0.0f;
    }
}

void RenderLoginScreen() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    
    // Background
    DrawGradientRect(drawList, ImVec2(0, 0), windowSize, 
        IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(drawList);
    
    // Fade in animation
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    // Center content
    float contentWidth = 340.0f;
    float startX = (windowSize.x - contentWidth) * 0.5f;
    float startY = 80.0f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("##login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Logo
        DrawLogo(drawList, ImVec2(windowSize.x * 0.5f, startY + 40), 40.0f);
        
        // Title
        ImGui::SetCursorPos(ImVec2(startX, startY + 100));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        
        float titleWidth = ImGui::CalcTextSize("Welcome Back").x;
        ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
        ImGui::TextColored(theme::text, "Welcome Back");
        
        float subWidth = ImGui::CalcTextSize("Sign in to continue").x;
        ImGui::SetCursorPosX((windowSize.x - subWidth) * 0.5f);
        ImGui::TextColored(theme::textSecondary, "Sign in to continue");
        ImGui::PopFont();
        
        ImGui::Dummy(ImVec2(0, 30));
        
        // Input fields
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Username");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentWidth);
        StyledInputText("##user", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 12));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Password");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentWidth);
        StyledInputText("##pass", g_password, sizeof(g_password), true);
        
        ImGui::Dummy(ImVec2(0, 24));
        
        // Login button
        ImGui::SetCursorPosX(startX);
        if (AnimatedButton(g_isLoading ? "  SIGNING IN...  " : "  SIGN IN  ", ImVec2(contentWidth, 50), !g_isLoading)) {
            DoLogin();
        }
        
        // Error message
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 12));
            float errWidth = ImGui::CalcTextSize(g_errorMsg.c_str()).x;
            ImGui::SetCursorPosX((windowSize.x - errWidth) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        
        // Status message
        if (!g_statusMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 12));
            float statusWidth = ImGui::CalcTextSize(g_statusMsg.c_str()).x;
            ImGui::SetCursorPosX((windowSize.x - statusWidth) * 0.5f);
            ImGui::TextColored(theme::secondary, "%s", g_statusMsg.c_str());
        }
        
        ImGui::Dummy(ImVec2(0, 30));
        
        // Register link
        float regWidth = ImGui::CalcTextSize("Don't have an account? Register").x;
        ImGui::SetCursorPosX((windowSize.x - regWidth) * 0.5f);
        ImGui::TextColored(theme::textDim, "Don't have an account?");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme::accent);
        if (ImGui::SmallButton("Register")) {
            g_currentScreen = Screen::Register;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
        }
        ImGui::PopStyleColor();
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void RenderRegisterScreen() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    
    DrawGradientRect(drawList, ImVec2(0, 0), windowSize, 
        IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(drawList);
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float contentWidth = 340.0f;
    float startX = (windowSize.x - contentWidth) * 0.5f;
    float startY = 60.0f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("##register", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        DrawLogo(drawList, ImVec2(windowSize.x * 0.5f, startY + 35), 35.0f);
        
        ImGui::SetCursorPos(ImVec2(startX, startY + 85));
        
        float titleWidth = ImGui::CalcTextSize("Create Account").x;
        ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
        ImGui::TextColored(theme::text, "Create Account");
        
        float subWidth = ImGui::CalcTextSize("Enter your license key to register").x;
        ImGui::SetCursorPosX((windowSize.x - subWidth) * 0.5f);
        ImGui::TextColored(theme::textSecondary, "Enter your license key to register");
        
        ImGui::Dummy(ImVec2(0, 25));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "License Key");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentWidth);
        StyledInputText("##key", g_licenseKey, sizeof(g_licenseKey));
        
        ImGui::Dummy(ImVec2(0, 10));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Username");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentWidth);
        StyledInputText("##ruser", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 10));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Password");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentWidth);
        StyledInputText("##rpass", g_password, sizeof(g_password), true);
        
        ImGui::Dummy(ImVec2(0, 20));
        
        ImGui::SetCursorPosX(startX);
        if (AnimatedButton(g_isLoading ? "  CREATING...  " : "  CREATE ACCOUNT  ", ImVec2(contentWidth, 50), !g_isLoading)) {
            DoRegister();
        }
        
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 12));
            float errWidth = ImGui::CalcTextSize(g_errorMsg.c_str()).x;
            ImGui::SetCursorPosX((windowSize.x - errWidth) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        
        ImGui::Dummy(ImVec2(0, 20));
        
        float backWidth = ImGui::CalcTextSize("Already have an account? Sign In").x;
        ImGui::SetCursorPosX((windowSize.x - backWidth) * 0.5f);
        ImGui::TextColored(theme::textDim, "Already have an account?");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme::accent);
        if (ImGui::SmallButton("Sign In")) {
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
        }
        ImGui::PopStyleColor();
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void RenderMainScreen() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    
    // Background
    DrawGradientRect(drawList, ImVec2(0, 0), windowSize, 
        IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(drawList);
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float contentWidth = 400.0f;
    float startX = (windowSize.x - contentWidth) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Header
        ImGui::SetCursorPos(ImVec2(24, 24));
        ImGui::TextColored(theme::textSecondary, "Logged in as");
        ImGui::SetCursorPos(ImVec2(24, 42));
        ImGui::TextColored(theme::text, "%s", g_displayName.c_str());
        
        // Logout button
        ImGui::SetCursorPos(ImVec2(windowSize.x - 90, 28));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("Logout", ImVec2(66, 32))) {
            ClearSession();
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // Game Card
        float cardY = 90;
        ImVec2 cardPos(startX - 20, cardY);
        ImVec2 cardSize(contentWidth + 40, 260);
        
        // Card background with glow
        DrawGlowRect(drawList, cardPos, cardSize, theme::gradientStart, 0.3f);
        drawList->AddRectFilled(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), 
            IM_COL32(18, 18, 28, 240), 16.0f);
        drawList->AddRect(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), 
            IM_COL32(60, 50, 100, 150), 16.0f, 0, 1.5f);
        
        // Game banner area (gradient placeholder)
        ImVec2 bannerPos(cardPos.x + 16, cardPos.y + 16);
        ImVec2 bannerSize(cardSize.x - 32, 100);
        DrawGradientRect(drawList, bannerPos, bannerSize, 
            theme::gradientStart, theme::gradientEnd);
        drawList->AddRect(bannerPos, ImVec2(bannerPos.x + bannerSize.x, bannerPos.y + bannerSize.y), 
            IM_COL32(255, 255, 255, 30), 12.0f);
        
        // CS2 Logo text
        const char* gameTitle = "COUNTER-STRIKE 2";
        ImVec2 gtSize = ImGui::CalcTextSize(gameTitle);
        drawList->AddText(
            ImVec2(bannerPos.x + (bannerSize.x - gtSize.x) * 0.5f, bannerPos.y + 35),
            IM_COL32(255, 255, 255, 255), gameTitle
        );
        
        const char* gameSubtitle = "External ESP";
        ImVec2 gsSize = ImGui::CalcTextSize(gameSubtitle);
        drawList->AddText(
            ImVec2(bannerPos.x + (bannerSize.x - gsSize.x) * 0.5f, bannerPos.y + 58),
            IM_COL32(255, 255, 255, 180), gameSubtitle
        );
        
        // Status info
        ImGui::SetCursorPos(ImVec2(cardPos.x + 20, cardPos.y + 130));
        ImGui::TextColored(theme::textSecondary, "Status:");
        ImGui::SameLine();
        ImGui::TextColored(theme::success, "ACTIVE");
        
        ImGui::SetCursorPos(ImVec2(cardPos.x + cardSize.x - 140, cardPos.y + 130));
        ImGui::TextColored(theme::textSecondary, "Version:");
        ImGui::SameLine();
        ImGui::TextColored(theme::secondary, "%s", g_cheatVersion.c_str());
        
        // Launch button
        ImGui::SetCursorPos(ImVec2(cardPos.x + 20, cardPos.y + 170));
        
        if (g_isDownloading) {
            // Progress bar
            float barWidth = cardSize.x - 40;
            ImVec2 barPos2(cardPos.x + 20, cardPos.y + 180);
            drawList->AddRectFilled(barPos2, ImVec2(barPos2.x + barWidth, barPos2.y + 40), 
                IM_COL32(30, 30, 45, 255), 10.0f);
            drawList->AddRectFilled(barPos2, ImVec2(barPos2.x + barWidth * g_downloadProgress, barPos2.y + 40), 
                theme::gradientStart, 10.0f);
            
            // Progress text
            char progressText[32];
            sprintf_s(progressText, "%.0f%%", g_downloadProgress * 100.0f);
            ImVec2 ptSize = ImGui::CalcTextSize(progressText);
            drawList->AddText(
                ImVec2(barPos2.x + (barWidth - ptSize.x) * 0.5f, barPos2.y + 10),
                IM_COL32(255, 255, 255, 255), progressText
            );
        } else {
            if (AnimatedButton("     LAUNCH     ", ImVec2(cardSize.x - 40, 50))) {
                LaunchCheat();
            }
        }
        
        // Status message
        if (!g_statusMsg.empty() && g_isDownloading) {
            ImGui::SetCursorPos(ImVec2(cardPos.x + 20, cardPos.y + 235));
            ImGui::TextColored(theme::secondary, "%s", g_statusMsg.c_str());
        }
        
        // Features section
        float featY = cardY + cardSize.y + 30;
        
        ImGui::SetCursorPos(ImVec2(startX, featY));
        ImGui::TextColored(theme::textSecondary, "FEATURES");
        
        float featureCardWidth = (contentWidth - 20) / 3;
        DrawFeatureCard(drawList, ImVec2(startX, featY + 25), "ESP", "Wallhack", "See enemies");
        DrawFeatureCard(drawList, ImVec2(startX + featureCardWidth + 10, featY + 25), "AIM", "Aimbot", "Precision aim");
        DrawFeatureCard(drawList, ImVec2(startX + (featureCardWidth + 10) * 2, featY + 25), "MISC", "Extras", "More tools");
        
        // Footer
        ImGui::SetCursorPos(ImVec2(0, windowSize.y - 40));
        float footerWidth = ImGui::CalcTextSize("CS-Legit v3.0 | Support: discord.gg/cslegit").x;
        ImGui::SetCursorPosX((windowSize.x - footerWidth) * 0.5f);
        ImGui::TextColored(theme::textDim, "CS-Legit v3.0 | Support: discord.gg/cslegit");
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ============================================
// Main Application
// ============================================
void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 10);
    style.ItemSpacing = ImVec2(10, 8);
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = theme::background;
    colors[ImGuiCol_ChildBg] = theme::surface;
    colors[ImGuiCol_PopupBg] = theme::surface;
    colors[ImGuiCol_Border] = theme::surfaceBorder;
    colors[ImGuiCol_FrameBg] = theme::surface;
    colors[ImGuiCol_FrameBgHovered] = theme::surfaceHover;
    colors[ImGuiCol_FrameBgActive] = theme::surfaceHover;
    colors[ImGuiCol_TitleBg] = theme::surface;
    colors[ImGuiCol_TitleBgActive] = theme::surface;
    colors[ImGuiCol_MenuBarBg] = theme::surface;
    colors[ImGuiCol_ScrollbarBg] = theme::background;
    colors[ImGuiCol_ScrollbarGrab] = theme::surfaceBorder;
    colors[ImGuiCol_ScrollbarGrabHovered] = theme::accent;
    colors[ImGuiCol_ScrollbarGrabActive] = theme::accentHover;
    colors[ImGuiCol_CheckMark] = theme::accent;
    colors[ImGuiCol_SliderGrab] = theme::accent;
    colors[ImGuiCol_SliderGrabActive] = theme::accentHover;
    colors[ImGuiCol_Button] = theme::accent;
    colors[ImGuiCol_ButtonHovered] = theme::accentHover;
    colors[ImGuiCol_ButtonActive] = theme::accent;
    colors[ImGuiCol_Header] = theme::surface;
    colors[ImGuiCol_HeaderHovered] = theme::surfaceHover;
    colors[ImGuiCol_HeaderActive] = theme::accent;
    colors[ImGuiCol_Tab] = theme::surface;
    colors[ImGuiCol_TabHovered] = theme::accentHover;
    colors[ImGuiCol_TabActive] = theme::accent;
    colors[ImGuiCol_Text] = theme::text;
    colors[ImGuiCol_TextDisabled] = theme::textDim;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Create window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, 
        L"CS-Legit Launcher", nullptr };
    RegisterClassExW(&wc);
    
    g_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName, L"CS-LEGIT",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    // Make window rounded (Windows 11)
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(g_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    
    // Enable transparency
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(g_hwnd, &margins);
    
    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    
    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Load fonts
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    SetupImGuiStyle();
    InitParticles();
    
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update animations
        g_animTimer += deltaTime;
        g_pulseValue = (sinf(g_animTimer * 3.0f) + 1.0f) * 0.5f;
        
        if (g_currentScreen == Screen::Splash) {
            g_splashTimer += deltaTime;
        }
        
        UpdateParticles(deltaTime);
        
        // Render
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        switch (g_currentScreen) {
            case Screen::Splash:
                RenderSplashScreen();
                break;
            case Screen::Login:
                RenderLoginScreen();
                break;
            case Screen::Register:
                RenderRegisterScreen();
                break;
            case Screen::Main:
                RenderMainScreen();
                break;
        }
        
        ImGui::Render();
        
        const float clear_color[4] = { 0.04f, 0.04f, 0.06f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        g_pSwapChain->Present(1, 0);
    }
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    CleanupDeviceD3D();
    DestroyWindow(g_hwnd);
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
    
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    
    if (res != S_OK) return false;
    
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

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
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
        case WM_NCHITTEST: {
            // Enable dragging
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < 70) return HTCAPTION;
            break;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
