/*
 * CS-LEGIT Launcher v1.0.0
 * Professional Gaming Software
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
#define LAUNCHER_VERSION "1.0.0"
#define CHEAT_VERSION "1.0.0"
#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 620

#define SERVER_HOST "138.124.0.8"
#define SERVER_PORT 80

// ============================================
// App State
// ============================================
enum class Screen { Splash, Login, Register, Main };

Screen g_currentScreen = Screen::Splash;
char g_username[64] = "";
char g_password[64] = "";
char g_licenseKey[128] = "";
char g_activateLicenseKey[128] = "";
std::string g_errorMsg = "";
std::string g_statusMsg = "";
std::string g_successMsg = "";
std::string g_token = "";

std::atomic<bool> g_isLoading{false};
std::atomic<float> g_downloadProgress{0.0f};
std::atomic<bool> g_isDownloading{false};
std::atomic<bool> g_cheatRunning{false};

// Animation
float g_animTimer = 0.0f;
float g_splashTimer = 0.0f;
float g_fadeAlpha = 0.0f;

// User data
std::string g_displayName = "";
std::string g_hwid = "";
bool g_hasValidLicense = false;
bool g_hwidBound = false;
std::string g_licenseExpiry = "";
int g_daysRemaining = -1; // -1 = lifetime

// Particles
struct Particle { float x, y, speed, size, alpha; };
std::vector<Particle> g_particles;

// ============================================
// Theme Colors
// ============================================
namespace theme {
    const ImVec4 background = ImVec4(0.04f, 0.04f, 0.06f, 1.0f);
    const ImVec4 surface = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    const ImVec4 surfaceHover = ImVec4(0.10f, 0.10f, 0.15f, 1.0f);
    const ImVec4 surfaceBorder = ImVec4(0.15f, 0.15f, 0.22f, 1.0f);
    
    const ImVec4 accent = ImVec4(0.55f, 0.36f, 0.95f, 1.0f);
    const ImVec4 accentHover = ImVec4(0.65f, 0.46f, 1.0f, 1.0f);
    
    const ImVec4 success = ImVec4(0.15f, 0.85f, 0.45f, 1.0f);
    const ImVec4 warning = ImVec4(1.0f, 0.70f, 0.0f, 1.0f);
    const ImVec4 error = ImVec4(0.95f, 0.25f, 0.30f, 1.0f);
    
    const ImVec4 text = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
    const ImVec4 textSecondary = ImVec4(0.60f, 0.60f, 0.68f, 1.0f);
    const ImVec4 textDim = ImVec4(0.40f, 0.40f, 0.48f, 1.0f);
    
    const ImU32 gradientStart = IM_COL32(140, 90, 245, 255);
    const ImU32 gradientEnd = IM_COL32(60, 180, 255, 255);
}

// ============================================
// DirectX
// ============================================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hwnd = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================
// Utility Functions
// ============================================
std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string appPath = std::string(path) + "\\CS-Legit";
        CreateDirectoryA(appPath.c_str(), NULL);
        return appPath;
    }
    return ".";
}

std::string GetHWID() {
    char volumeName[MAX_PATH], fsName[MAX_PATH];
    DWORD serialNumber, maxLen, flags;
    if (GetVolumeInformationA("C:\\", volumeName, MAX_PATH, &serialNumber, &maxLen, &flags, fsName, MAX_PATH)) {
        char hwid[32];
        sprintf_s(hwid, "%08X", serialNumber);
        return std::string(hwid);
    }
    return "UNKNOWN";
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
    g_hasValidLicense = false;
    g_hwidBound = false;
    g_daysRemaining = -1;
    DeleteFileA((GetAppDataPath() + "\\session.dat").c_str());
}

// Particles
void InitParticles() {
    g_particles.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(0.0f, (float)WINDOW_WIDTH);
    std::uniform_real_distribution<float> distY(0.0f, (float)WINDOW_HEIGHT);
    std::uniform_real_distribution<float> distSpeed(10.0f, 30.0f);
    std::uniform_real_distribution<float> distSize(1.0f, 2.5f);
    std::uniform_real_distribution<float> distAlpha(0.1f, 0.3f);
    
    for (int i = 0; i < 40; i++) {
        Particle p;
        p.x = distX(gen);
        p.y = distY(gen);
        p.speed = distSpeed(gen);
        p.size = distSize(gen);
        p.alpha = distAlpha(gen);
        g_particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    for (auto& p : g_particles) {
        p.y -= p.speed * dt;
        if (p.y < -10) {
            p.y = WINDOW_HEIGHT + 10;
            p.x = (float)(rand() % WINDOW_WIDTH);
        }
    }
}

void DrawParticles(ImDrawList* dl) {
    for (const auto& p : g_particles) {
        dl->AddCircleFilled(ImVec2(p.x, p.y), p.size, IM_COL32(140, 90, 245, (int)(p.alpha * 255)));
    }
}

void DrawGradientRect(ImDrawList* dl, ImVec2 pos, ImVec2 size, ImU32 top, ImU32 bottom) {
    dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y), top, top, bottom, bottom);
}

// ============================================
// HTTP API
// ============================================
std::string HttpRequest(const std::string& method, const std::string& path, const std::string& data = "") {
    HINTERNET hInternet = InternetOpenA("CS-Legit", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";
    
    HINTERNET hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return ""; }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, 0, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return ""; }
    
    std::string headers = "Content-Type: application/json\r\n";
    if (!g_token.empty()) headers += "Authorization: Bearer " + g_token + "\r\n";
    
    BOOL result;
    if (data.empty()) {
        result = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), NULL, 0);
    } else {
        result = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)data.c_str(), (DWORD)data.length());
    }
    
    if (!result) {
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

std::string ExtractJson(const std::string& json, const std::string& key) {
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

bool ExtractBool(const std::string& json, const std::string& key) {
    std::string val = ExtractJson(json, key);
    return val == "true" || val == "1";
}

int ExtractInt(const std::string& json, const std::string& key) {
    std::string val = ExtractJson(json, key);
    try { return std::stoi(val); } catch (...) { return 0; }
}

// ============================================
// API Functions
// ============================================
void FetchUserInfo() {
    std::string response = HttpRequest("GET", "/api/auth/me");
    if (response.empty()) return;
    
    g_hasValidLicense = ExtractBool(response, "has_license");
    g_hwidBound = !ExtractJson(response, "hwid").empty();
    g_licenseExpiry = ExtractJson(response, "license_expires");
    g_daysRemaining = ExtractInt(response, "days_remaining");
}

void DoLogin() {
    if (strlen(g_username) == 0 || strlen(g_password) == 0) {
        g_errorMsg = "Enter username and password";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    g_statusMsg = "Connecting...";
    
    std::thread([]() {
        g_hwid = GetHWID();
        std::string data = "{\"username\":\"" + std::string(g_username) + 
                          "\",\"password\":\"" + std::string(g_password) + 
                          "\",\"hwid\":\"" + g_hwid + "\"}";
        
        std::string response = HttpRequest("POST", "/api/auth/login", data);
        
        if (response.empty()) {
            g_errorMsg = "Connection failed";
            g_isLoading = false;
            return;
        }
        
        if (response.find("\"token\"") != std::string::npos) {
            g_token = ExtractJson(response, "token");
            g_displayName = g_username;
            g_hasValidLicense = ExtractBool(response, "has_license");
            g_hwidBound = ExtractBool(response, "hwid_bound");
            g_daysRemaining = ExtractInt(response, "days_remaining");
            
            SaveSession(g_token, g_displayName);
            g_currentScreen = Screen::Main;
            g_fadeAlpha = 0.0f;
            g_successMsg = "Welcome, " + g_displayName + "!";
        } else {
            g_errorMsg = ExtractJson(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Login failed";
        }
        
        g_isLoading = false;
        g_statusMsg = "";
    }).detach();
}

void DoRegister() {
    if (strlen(g_username) == 0 || strlen(g_password) == 0 || strlen(g_licenseKey) == 0) {
        g_errorMsg = "Fill all fields";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    g_statusMsg = "Creating account...";
    
    std::thread([]() {
        std::string data = "{\"username\":\"" + std::string(g_username) + 
                          "\",\"password\":\"" + std::string(g_password) + 
                          "\",\"license_key\":\"" + std::string(g_licenseKey) + "\"}";
        
        std::string response = HttpRequest("POST", "/api/auth/register", data);
        
        if (response.empty()) {
            g_errorMsg = "Connection failed";
            g_isLoading = false;
            return;
        }
        
        if (response.find("\"success\"") != std::string::npos || response.find("\"token\"") != std::string::npos) {
            g_successMsg = "Account created! Please login.";
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
            memset(g_licenseKey, 0, sizeof(g_licenseKey));
        } else {
            g_errorMsg = ExtractJson(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Registration failed";
        }
        
        g_isLoading = false;
        g_statusMsg = "";
    }).detach();
}

void ActivateLicense() {
    if (strlen(g_activateLicenseKey) == 0) {
        g_errorMsg = "Enter license key";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    g_statusMsg = "Activating...";
    
    std::thread([]() {
        std::string data = "{\"license_key\":\"" + std::string(g_activateLicenseKey) + "\"}";
        std::string response = HttpRequest("POST", "/api/auth/activate", data);
        
        if (response.empty()) {
            g_errorMsg = "Connection failed";
            g_isLoading = false;
            return;
        }
        
        if (response.find("\"success\"") != std::string::npos) {
            g_successMsg = "License activated!";
            g_hasValidLicense = true;
            g_daysRemaining = ExtractInt(response, "days_remaining");
            memset(g_activateLicenseKey, 0, sizeof(g_activateLicenseKey));
            FetchUserInfo();
        } else {
            g_errorMsg = ExtractJson(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Activation failed";
        }
        
        g_isLoading = false;
        g_statusMsg = "";
    }).detach();
}

bool DownloadFile(const std::string& url, const std::string& savePath) {
    HINTERNET hInternet = InternetOpenA("CS-Legit", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;
    
    HINTERNET hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return false; }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", url.c_str(), NULL, NULL, NULL, 0, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return false; }
    
    std::string headers = "Authorization: Bearer " + g_token + "\r\n";
    if (!HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    // Get file size
    DWORD contentLength = 0;
    DWORD bufLen = sizeof(contentLength);
    HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &bufLen, NULL);
    
    std::ofstream file(savePath, std::ios::binary);
    if (!file.is_open()) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    char buffer[8192];
    DWORD bytesRead;
    DWORD totalRead = 0;
    
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        file.write(buffer, bytesRead);
        totalRead += bytesRead;
        if (contentLength > 0) {
            g_downloadProgress = (float)totalRead / (float)contentLength;
        }
    }
    
    file.close();
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return totalRead > 0;
}

void LaunchCheat() {
    if (!g_hasValidLicense) {
        g_errorMsg = "No active license!";
        return;
    }
    
    g_isDownloading = true;
    g_downloadProgress = 0.0f;
    g_statusMsg = "Downloading...";
    g_errorMsg = "";
    
    std::thread([]() {
        std::string savePath = GetAppDataPath() + "\\cs2_external.exe";
        
        if (DownloadFile("/api/download/cs2/external", savePath)) {
            g_statusMsg = "Launching...";
            Sleep(500);
            
            // Check if CS2 is running
            bool cs2Running = false;
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe;
                pe.dwSize = sizeof(pe);
                if (Process32First(hSnap, &pe)) {
                    do {
                        if (_stricmp(pe.szExeFile, "cs2.exe") == 0) {
                            cs2Running = true;
                            break;
                        }
                    } while (Process32Next(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }
            
            if (!cs2Running) {
                g_errorMsg = "Start CS2 first!";
                g_isDownloading = false;
                return;
            }
            
            // Launch cheat
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            if (CreateProcessA(savePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                g_cheatRunning = true;
                g_successMsg = "Cheat running!";
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else {
                g_errorMsg = "Failed to launch cheat";
            }
        } else {
            g_errorMsg = "Download failed";
        }
        
        g_isDownloading = false;
        g_statusMsg = "";
    }).detach();
}

void DoLogout() {
    ClearSession();
    g_currentScreen = Screen::Login;
    g_fadeAlpha = 0.0f;
    g_successMsg = "";
    g_errorMsg = "";
    memset(g_username, 0, sizeof(g_username));
    memset(g_password, 0, sizeof(g_password));
}

// ============================================
// UI Components
// ============================================
void StyledInput(const char* label, char* buf, size_t size, bool password = false) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 12));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme::surface);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme::surfaceHover);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::surfaceBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    
    ImGui::InputText(label, buf, size, password ? ImGuiInputTextFlags_Password : 0);
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}

bool StyledButton(const char* label, ImVec2 size, bool enabled = true) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    
    if (!enabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, theme::accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::accentHover);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    }
    
    bool clicked = ImGui::Button(label, size);
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    return clicked && enabled;
}

// ============================================
// Screen Renders
// ============================================
void RenderSplash() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    ImVec2 center(ws.x * 0.5f, ws.y * 0.5f);
    
    DrawGradientRect(dl, ImVec2(0, 0), ws, IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(dl);
    
    // Logo
    float scale = fminf(g_splashTimer / 0.5f, 1.0f);
    dl->AddCircleFilled(ImVec2(center.x, center.y - 40), 45.0f * scale, theme::gradientStart, 32);
    
    if (g_splashTimer > 0.3f) {
        float alpha = fminf((g_splashTimer - 0.3f) / 0.4f, 1.0f);
        ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * 255));
        
        const char* title = "CS-LEGIT";
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y + 30), col, title);
        
        const char* ver = "v" LAUNCHER_VERSION;
        ImVec2 vs = ImGui::CalcTextSize(ver);
        dl->AddText(ImVec2(center.x - vs.x * 0.5f, center.y + 55), IM_COL32(140, 90, 245, (int)(alpha * 200)), ver);
    }
    
    // Progress bar
    float barW = 180.0f;
    float progress = fminf(g_splashTimer / 2.0f, 1.0f);
    ImVec2 barPos(center.x - barW * 0.5f, center.y + 90);
    dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + 4), IM_COL32(40, 40, 60, 255), 2.0f);
    dl->AddRectFilled(barPos, ImVec2(barPos.x + barW * progress, barPos.y + 4), theme::gradientStart, 2.0f);
    
    if (g_splashTimer > 2.2f) {
        if (LoadSession()) {
            FetchUserInfo();
            g_currentScreen = Screen::Main;
        } else {
            g_currentScreen = Screen::Login;
        }
        g_fadeAlpha = 0.0f;
    }
}

void RenderLogin() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    
    DrawGradientRect(dl, ImVec2(0, 0), ws, IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(dl);
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float contentW = 320.0f;
    float startX = (ws.x - contentW) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    if (ImGui::Begin("##login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Logo
        dl->AddCircleFilled(ImVec2(ws.x * 0.5f, 100), 35.0f, theme::gradientStart, 32);
        
        ImGui::SetCursorPos(ImVec2(startX, 160));
        
        float tw = ImGui::CalcTextSize("Sign In").x;
        ImGui::SetCursorPosX((ws.x - tw) * 0.5f);
        ImGui::TextColored(theme::text, "Sign In");
        
        ImGui::Dummy(ImVec2(0, 30));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Username");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentW);
        StyledInput("##user", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 10));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Password");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentW);
        StyledInput("##pass", g_password, sizeof(g_password), true);
        
        ImGui::Dummy(ImVec2(0, 20));
        
        ImGui::SetCursorPosX(startX);
        if (StyledButton(g_isLoading ? "  Signing in...  " : "  SIGN IN  ", ImVec2(contentW, 48), !g_isLoading)) {
            DoLogin();
        }
        
        // Messages
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            float ew = ImGui::CalcTextSize(g_errorMsg.c_str()).x;
            ImGui::SetCursorPosX((ws.x - ew) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        
        if (!g_successMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            float sw = ImGui::CalcTextSize(g_successMsg.c_str()).x;
            ImGui::SetCursorPosX((ws.x - sw) * 0.5f);
            ImGui::TextColored(theme::success, "%s", g_successMsg.c_str());
        }
        
        ImGui::Dummy(ImVec2(0, 25));
        
        float lw = ImGui::CalcTextSize("No account?").x + ImGui::CalcTextSize("Register").x + 10;
        ImGui::SetCursorPosX((ws.x - lw) * 0.5f);
        ImGui::TextColored(theme::textDim, "No account?");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme::accent);
        if (ImGui::SmallButton("Register")) {
            g_currentScreen = Screen::Register;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
            g_successMsg = "";
        }
        ImGui::PopStyleColor(3);
        
        // Version
        ImGui::SetCursorPos(ImVec2(0, ws.y - 30));
        float vw = ImGui::CalcTextSize("v" LAUNCHER_VERSION).x;
        ImGui::SetCursorPosX((ws.x - vw) * 0.5f);
        ImGui::TextColored(theme::textDim, "v" LAUNCHER_VERSION);
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void RenderRegister() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    
    DrawGradientRect(dl, ImVec2(0, 0), ws, IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(dl);
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float contentW = 320.0f;
    float startX = (ws.x - contentW) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    if (ImGui::Begin("##reg", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        dl->AddCircleFilled(ImVec2(ws.x * 0.5f, 80), 30.0f, theme::gradientStart, 32);
        
        ImGui::SetCursorPos(ImVec2(startX, 130));
        
        float tw = ImGui::CalcTextSize("Create Account").x;
        ImGui::SetCursorPosX((ws.x - tw) * 0.5f);
        ImGui::TextColored(theme::text, "Create Account");
        
        ImGui::Dummy(ImVec2(0, 25));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "License Key");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentW);
        StyledInput("##key", g_licenseKey, sizeof(g_licenseKey));
        
        ImGui::Dummy(ImVec2(0, 8));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Username");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentW);
        StyledInput("##ruser", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 8));
        
        ImGui::SetCursorPosX(startX);
        ImGui::TextColored(theme::textSecondary, "Password");
        ImGui::SetCursorPosX(startX);
        ImGui::SetNextItemWidth(contentW);
        StyledInput("##rpass", g_password, sizeof(g_password), true);
        
        ImGui::Dummy(ImVec2(0, 18));
        
        ImGui::SetCursorPosX(startX);
        if (StyledButton(g_isLoading ? "  Creating...  " : "  CREATE ACCOUNT  ", ImVec2(contentW, 48), !g_isLoading)) {
            DoRegister();
        }
        
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            float ew = ImGui::CalcTextSize(g_errorMsg.c_str()).x;
            ImGui::SetCursorPosX((ws.x - ew) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        
        ImGui::Dummy(ImVec2(0, 20));
        
        float lw = ImGui::CalcTextSize("Have account?").x + ImGui::CalcTextSize("Sign In").x + 10;
        ImGui::SetCursorPosX((ws.x - lw) * 0.5f);
        ImGui::TextColored(theme::textDim, "Have account?");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme::accent);
        if (ImGui::SmallButton("Sign In")) {
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
        }
        ImGui::PopStyleColor(3);
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void RenderMain() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    
    DrawGradientRect(dl, ImVec2(0, 0), ws, IM_COL32(10, 10, 18, 255), IM_COL32(5, 5, 10, 255));
    DrawParticles(dl);
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float contentW = 400.0f;
    float startX = (ws.x - contentW) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    if (ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Header
        ImGui::SetCursorPos(ImVec2(20, 20));
        ImGui::TextColored(theme::textSecondary, "Welcome,");
        ImGui::SetCursorPos(ImVec2(20, 38));
        ImGui::TextColored(theme::text, "%s", g_displayName.c_str());
        
        // Logout button
        ImGui::SetCursorPos(ImVec2(ws.x - 80, 25));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("Logout", ImVec2(60, 30))) {
            DoLogout();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            ImGui::End();
            ImGui::PopStyleColor();
            return;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // License Status Card
        float cardY = 75;
        ImVec2 cardPos(startX, cardY);
        ImVec2 cardSize(contentW, 80);
        
        dl->AddRectFilled(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), 
            IM_COL32(20, 20, 32, 240), 12.0f);
        dl->AddRect(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), 
            IM_COL32(60, 50, 90, 150), 12.0f);
        
        ImGui::SetCursorPos(ImVec2(cardPos.x + 16, cardPos.y + 14));
        ImGui::TextColored(theme::textSecondary, "License Status");
        
        ImGui::SetCursorPos(ImVec2(cardPos.x + 16, cardPos.y + 38));
        if (g_hasValidLicense) {
            ImGui::TextColored(theme::success, "ACTIVE");
            ImGui::SameLine();
            if (g_daysRemaining < 0) {
                ImGui::TextColored(theme::textSecondary, "- Lifetime");
            } else {
                ImGui::TextColored(theme::textSecondary, "- %d days remaining", g_daysRemaining);
            }
        } else {
            ImGui::TextColored(theme::error, "NO LICENSE");
        }
        
        // HWID Status
        ImGui::SetCursorPos(ImVec2(cardPos.x + cardSize.x - 120, cardPos.y + 38));
        ImGui::TextColored(theme::textDim, "HWID: %s", g_hwidBound ? "Bound" : "Not bound");
        
        // Activate License Section (if no license)
        if (!g_hasValidLicense) {
            float actY = cardY + cardSize.y + 15;
            ImVec2 actPos(startX, actY);
            ImVec2 actSize(contentW, 100);
            
            dl->AddRectFilled(actPos, ImVec2(actPos.x + actSize.x, actPos.y + actSize.y), 
                IM_COL32(25, 20, 35, 240), 12.0f);
            dl->AddRect(actPos, ImVec2(actPos.x + actSize.x, actPos.y + actSize.y), 
                IM_COL32(80, 60, 120, 150), 12.0f);
            
            ImGui::SetCursorPos(ImVec2(actPos.x + 16, actPos.y + 14));
            ImGui::TextColored(theme::accent, "Activate License");
            
            ImGui::SetCursorPos(ImVec2(actPos.x + 16, actPos.y + 40));
            ImGui::SetNextItemWidth(actSize.x - 130);
            StyledInput("##actkey", g_activateLicenseKey, sizeof(g_activateLicenseKey));
            
            ImGui::SameLine();
            if (StyledButton("Activate", ImVec2(90, 36), !g_isLoading)) {
                ActivateLicense();
            }
        }
        
        // Game Card
        float gameY = g_hasValidLicense ? (cardY + cardSize.y + 20) : (cardY + cardSize.y + 130);
        ImVec2 gamePos(startX, gameY);
        ImVec2 gameSize(contentW, 200);
        
        dl->AddRectFilled(gamePos, ImVec2(gamePos.x + gameSize.x, gamePos.y + gameSize.y), 
            IM_COL32(18, 18, 28, 240), 14.0f);
        dl->AddRect(gamePos, ImVec2(gamePos.x + gameSize.x, gamePos.y + gameSize.y), 
            IM_COL32(60, 50, 100, 150), 14.0f);
        
        // Game banner
        ImVec2 bannerPos(gamePos.x + 14, gamePos.y + 14);
        ImVec2 bannerSize(gameSize.x - 28, 70);
        DrawGradientRect(dl, bannerPos, bannerSize, theme::gradientStart, theme::gradientEnd);
        dl->AddRect(bannerPos, ImVec2(bannerPos.x + bannerSize.x, bannerPos.y + bannerSize.y), 
            IM_COL32(255, 255, 255, 30), 10.0f);
        
        const char* gameTitle = "COUNTER-STRIKE 2";
        ImVec2 gtSize = ImGui::CalcTextSize(gameTitle);
        dl->AddText(ImVec2(bannerPos.x + (bannerSize.x - gtSize.x) * 0.5f, bannerPos.y + 18), 
            IM_COL32(255, 255, 255, 255), gameTitle);
        
        const char* gameVer = "External v" CHEAT_VERSION;
        ImVec2 gvSize = ImGui::CalcTextSize(gameVer);
        dl->AddText(ImVec2(bannerPos.x + (bannerSize.x - gvSize.x) * 0.5f, bannerPos.y + 42), 
            IM_COL32(255, 255, 255, 180), gameVer);
        
        // Features
        ImGui::SetCursorPos(ImVec2(gamePos.x + 16, gamePos.y + 100));
        ImGui::TextColored(theme::textSecondary, "Features:");
        
        ImGui::SetCursorPos(ImVec2(gamePos.x + 90, gamePos.y + 100));
        ImGui::TextColored(theme::success, "ESP");
        ImGui::SameLine();
        ImGui::TextColored(theme::textDim, "|");
        ImGui::SameLine();
        ImGui::TextColored(theme::success, "MISC");
        
        // Launch button
        ImGui::SetCursorPos(ImVec2(gamePos.x + 14, gamePos.y + 140));
        
        if (g_isDownloading) {
            // Progress bar
            float barW = gameSize.x - 28;
            ImVec2 barPos2(gamePos.x + 14, gamePos.y + 150);
            dl->AddRectFilled(barPos2, ImVec2(barPos2.x + barW, barPos2.y + 36), 
                IM_COL32(30, 30, 45, 255), 10.0f);
            dl->AddRectFilled(barPos2, ImVec2(barPos2.x + barW * g_downloadProgress, barPos2.y + 36), 
                theme::gradientStart, 10.0f);
            
            char progText[32];
            sprintf_s(progText, "%.0f%%", g_downloadProgress * 100.0f);
            ImVec2 ptSize = ImGui::CalcTextSize(progText);
            dl->AddText(ImVec2(barPos2.x + (barW - ptSize.x) * 0.5f, barPos2.y + 8), 
                IM_COL32(255, 255, 255, 255), progText);
        } else {
            bool canLaunch = g_hasValidLicense && !g_cheatRunning;
            if (StyledButton(g_cheatRunning ? "  RUNNING  " : "  LAUNCH  ", 
                ImVec2(gameSize.x - 28, 46), canLaunch)) {
                LaunchCheat();
            }
        }
        
        // Messages
        float msgY = gameY + gameSize.y + 15;
        if (!g_errorMsg.empty()) {
            float ew = ImGui::CalcTextSize(g_errorMsg.c_str()).x;
            ImGui::SetCursorPos(ImVec2((ws.x - ew) * 0.5f, msgY));
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        if (!g_successMsg.empty()) {
            float sw = ImGui::CalcTextSize(g_successMsg.c_str()).x;
            ImGui::SetCursorPos(ImVec2((ws.x - sw) * 0.5f, msgY));
            ImGui::TextColored(theme::success, "%s", g_successMsg.c_str());
        }
        
        // Footer
        ImGui::SetCursorPos(ImVec2(0, ws.y - 30));
        float fw = ImGui::CalcTextSize("CS-Legit v" LAUNCHER_VERSION).x;
        ImGui::SetCursorPosX((ws.x - fw) * 0.5f);
        ImGui::TextColored(theme::textDim, "CS-Legit v" LAUNCHER_VERSION);
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

// ============================================
// Main
// ============================================
void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 10);
    style.ItemSpacing = ImVec2(10, 8);
    
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = theme::background;
    c[ImGuiCol_FrameBg] = theme::surface;
    c[ImGuiCol_FrameBgHovered] = theme::surfaceHover;
    c[ImGuiCol_Button] = theme::accent;
    c[ImGuiCol_ButtonHovered] = theme::accentHover;
    c[ImGuiCol_Text] = theme::text;
    c[ImGuiCol_TextDisabled] = theme::textDim;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, 
        L"CS-Legit", nullptr };
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
    
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(g_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    
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
    
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);
    
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    SetupStyle();
    InitParticles();
    g_hwid = GetHWID();
    
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        
        g_animTimer += dt;
        if (g_currentScreen == Screen::Splash) g_splashTimer += dt;
        UpdateParticles(dt);
        
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        switch (g_currentScreen) {
            case Screen::Splash: RenderSplash(); break;
            case Screen::Login: RenderLogin(); break;
            case Screen::Register: RenderRegister(); break;
            case Screen::Main: RenderMain(); break;
        }
        
        ImGui::Render();
        const float clear[4] = { 0.04f, 0.04f, 0.06f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
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
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
        0, levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
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

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_NCHITTEST: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < 60) return HTCAPTION;
            break;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
