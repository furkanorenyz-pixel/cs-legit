/*
 * Single-Project Launcher v1.0.0
 * Premium Gaming Software
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <winhttp.h>
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
#pragma comment(lib, "winhttp.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "protection.hpp"

namespace fs = std::filesystem;

// ============================================
// Protection flags
// ============================================
bool g_protectionPassed = false;
bool g_bypassVM = false; // Set to true for development

// ============================================
// Configuration
// ============================================
#define PROJECT_NAME "Single-Project"
#define LAUNCHER_VERSION "1.0.0"
#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 500

#define SERVER_HOST "single-project.duckdns.org"
#define SERVER_PORT 80

// ============================================
// Game Data
// ============================================
struct GameInfo {
    std::string id;
    std::string name;
    std::string icon;
    std::string processName;
    std::string description;
    bool hasLicense;
    int daysRemaining;
    std::string version;
    bool available;
};

std::vector<GameInfo> g_games = {
    {"cs2", "Counter-Strike 2", "CS", "cs2.exe", "ESP (Box, Health, Name, Distance)", false, 0, "1.0.0", true},
    {"dayz", "DayZ", "DZ", "DayZ_x64.exe", "In development", false, 0, "---", false},
    {"rust", "Rust", "RS", "RustClient.exe", "In development", false, 0, "---", false},
    {"apex", "Apex Legends", "AP", "r5apex.exe", "In development", false, 0, "---", false}
};

int g_selectedGame = 0;

// ============================================
// App State
// ============================================
enum class Screen { Splash, Login, Register, Main };

Screen g_currentScreen = Screen::Splash;
char g_username[64] = "";
char g_password[64] = "";
char g_licenseKey[128] = "";
char g_activateKey[128] = "";
bool g_rememberLogin = true;
std::string g_errorMsg = "";
std::string g_successMsg = "";
std::string g_statusMsg = "";
std::string g_token = "";

std::atomic<bool> g_isLoading{false};
std::atomic<float> g_downloadProgress{0.0f};
std::atomic<bool> g_isDownloading{false};
std::atomic<bool> g_cheatRunning{false};

float g_animTimer = 0.0f;
float g_splashTimer = 0.0f;
float g_fadeAlpha = 0.0f;

std::string g_displayName = "";
std::string g_hwid = "";

// ============================================
// Theme
// ============================================
namespace theme {
    const ImVec4 bg = ImVec4(0.04f, 0.04f, 0.06f, 1.0f);
    const ImVec4 sidebar = ImVec4(0.06f, 0.06f, 0.09f, 1.0f);
    const ImVec4 surface = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    const ImVec4 surfaceHover = ImVec4(0.12f, 0.12f, 0.18f, 1.0f);
    const ImVec4 border = ImVec4(0.15f, 0.15f, 0.22f, 1.0f);
    
    const ImVec4 accent = ImVec4(0.55f, 0.36f, 0.95f, 1.0f);
    const ImVec4 accentHover = ImVec4(0.65f, 0.46f, 1.0f, 1.0f);
    
    const ImVec4 success = ImVec4(0.15f, 0.85f, 0.45f, 1.0f);
    const ImVec4 warning = ImVec4(1.0f, 0.70f, 0.0f, 1.0f);
    const ImVec4 error = ImVec4(0.95f, 0.25f, 0.30f, 1.0f);
    
    const ImVec4 text = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
    const ImVec4 textSec = ImVec4(0.60f, 0.60f, 0.68f, 1.0f);
    const ImVec4 textDim = ImVec4(0.40f, 0.40f, 0.48f, 1.0f);
    
    const ImU32 gradientA = IM_COL32(140, 90, 245, 255);
    const ImU32 gradientB = IM_COL32(60, 180, 255, 255);
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
// Utilities
// ============================================
std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string appPath = std::string(path) + "\\SingleProject";
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

void SaveCredentials() {
    if (g_rememberLogin && strlen(g_username) > 0) {
        std::ofstream file(GetAppDataPath() + "\\credentials.dat");
        if (file.is_open()) {
            file << g_username << "\n" << g_password;
            file.close();
        }
    }
}

void LoadCredentials() {
    std::ifstream file(GetAppDataPath() + "\\credentials.dat");
    if (file.is_open()) {
        std::string user, pass;
        std::getline(file, user);
        std::getline(file, pass);
        file.close();
        if (!user.empty()) {
            strncpy_s(g_username, user.c_str(), sizeof(g_username) - 1);
            strncpy_s(g_password, pass.c_str(), sizeof(g_password) - 1);
        }
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
    for (auto& g : g_games) {
        g.hasLicense = false;
        g.daysRemaining = 0;
    }
    DeleteFileA((GetAppDataPath() + "\\session.dat").c_str());
}

// ============================================
// HTTP
// ============================================
std::string HttpRequest(const std::string& method, const std::string& path, const std::string& data = "") {
    HINTERNET hInternet = InternetOpenA("SingleProject", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";
    
    HINTERNET hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return ""; }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, 0, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return ""; }
    
    std::string headers = "Content-Type: application/json\r\n";
    if (!g_token.empty()) headers += "Authorization: Bearer " + g_token + "\r\n";
    
    BOOL result = data.empty() 
        ? HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), NULL, 0)
        : HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)data.c_str(), (DWORD)data.length());
    
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
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    size_t end = pos;
    bool inStr = json[pos - 1] == '"';
    if (inStr) end = json.find('"', pos);
    else while (end < json.length() && json[end] != ',' && json[end] != '}') end++;
    return json.substr(pos, end - pos);
}

int ExtractInt(const std::string& json, const std::string& key) {
    std::string val = ExtractJson(json, key);
    try { return std::stoi(val); } catch (...) { return 0; }
}

// ============================================
// API Functions
// ============================================
void FetchUserLicenses() {
    std::string response = HttpRequest("GET", "/api/auth/me");
    if (response.empty()) return;
    
    for (auto& game : g_games) {
        game.hasLicense = false;
        game.daysRemaining = 0;
    }
    
    if (response.find("\"cs2\"") != std::string::npos || response.find("\"game_id\":\"cs2\"") != std::string::npos) {
        g_games[0].hasLicense = true;
        g_games[0].daysRemaining = ExtractInt(response, "days_remaining");
    }
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
            SaveSession(g_token, g_displayName);
            SaveCredentials();
            FetchUserLicenses();
            g_currentScreen = Screen::Main;
            g_fadeAlpha = 0.0f;
            g_successMsg = "Welcome!";
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
        
        if (response.find("\"success\"") != std::string::npos) {
            g_successMsg = "Account created! Login now.";
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
            memset(g_licenseKey, 0, sizeof(g_licenseKey));
        } else {
            g_errorMsg = ExtractJson(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Registration failed";
        }
        
        g_isLoading = false;
    }).detach();
}

void ActivateLicense(const std::string& gameId) {
    if (strlen(g_activateKey) == 0) {
        g_errorMsg = "Enter license key";
        return;
    }
    
    g_isLoading = true;
    g_errorMsg = "";
    
    std::thread([gameId]() {
        std::string data = "{\"license_key\":\"" + std::string(g_activateKey) + "\"}";
        std::string response = HttpRequest("POST", "/api/auth/activate", data);
        
        if (response.find("\"success\"") != std::string::npos) {
            g_successMsg = "License activated!";
            memset(g_activateKey, 0, sizeof(g_activateKey));
            FetchUserLicenses();
        } else {
            g_errorMsg = ExtractJson(response, "error");
            if (g_errorMsg.empty()) g_errorMsg = "Activation failed";
        }
        
        g_isLoading = false;
    }).detach();
}

bool DownloadFile(const std::string& url, const std::string& savePath) {
    HINTERNET hInternet = InternetOpenA("SingleProject", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
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
    DWORD bytesRead, totalRead = 0;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        file.write(buffer, bytesRead);
        totalRead += bytesRead;
        if (contentLength > 0) g_downloadProgress = (float)totalRead / contentLength;
    }
    
    file.close();
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return totalRead > 0;
}

// Check game status from server
std::string GetGameStatus(const std::string& gameId) {
    HINTERNET hSession = WinHttpOpen(L"SingleProject/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "error";
    
    std::wstring wHost(SERVER_HOST, SERVER_HOST + strlen(SERVER_HOST));
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), SERVER_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return "error"; }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/games/status", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return "error"; }
    
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "error";
    }
    
    std::string response;
    DWORD bytesRead;
    char buffer[4096];
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    // Parse status for game
    std::string searchKey = "\"" + gameId + "\":{";
    size_t pos = response.find(searchKey);
    if (pos != std::string::npos) {
        size_t statusPos = response.find("\"status\":\"", pos);
        if (statusPos != std::string::npos) {
            statusPos += 10;
            size_t endPos = response.find("\"", statusPos);
            if (endPos != std::string::npos) {
                return response.substr(statusPos, endPos - statusPos);
            }
        }
    }
    
    return "operational";
}

void LaunchGame(int gameIndex) {
    GameInfo& game = g_games[gameIndex];
    
    if (!game.hasLicense) {
        g_errorMsg = "No license for " + game.name;
        return;
    }
    
    if (!game.available) {
        g_errorMsg = game.name + " coming soon!";
        return;
    }
    
    // Check game status before launching
    g_statusMsg = "Checking status...";
    std::string status = GetGameStatus(game.id);
    
    if (status == "updating") {
        g_errorMsg = "Cheat is being updated. Please wait...";
        g_statusMsg = "";
        return;
    }
    
    if (status == "maintenance" || status == "offline") {
        g_errorMsg = "Cheat is currently offline for maintenance";
        g_statusMsg = "";
        return;
    }
    
    if (status == "error") {
        g_errorMsg = "Cannot check status. Try again later.";
        g_statusMsg = "";
        return;
    }
    
    g_isDownloading = true;
    g_downloadProgress = 0.0f;
    g_statusMsg = "Downloading...";
    g_errorMsg = "";
    
    std::thread([gameIndex, &game]() {
        std::string savePath = GetAppDataPath() + "\\" + game.id + "_external.exe";
        std::string url = "/api/download/" + game.id + "/external";
        
        if (DownloadFile(url, savePath)) {
            g_statusMsg = "Checking game...";
            
            bool gameRunning = false;
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe = { sizeof(pe) };
                if (Process32First(hSnap, &pe)) {
                    do {
                        if (_stricmp(pe.szExeFile, game.processName.c_str()) == 0) {
                            gameRunning = true;
                            break;
                        }
                    } while (Process32Next(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }
            
            if (!gameRunning) {
                g_errorMsg = "Start " + game.name + " first!";
                g_isDownloading = false;
                return;
            }
            
            g_statusMsg = "Launching...";
            Sleep(500);
            
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            if (CreateProcessA(savePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                g_cheatRunning = true;
                g_successMsg = "Running!";
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else {
                g_errorMsg = "Launch failed";
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
    g_cheatRunning = false;
}

// ============================================
// UI Components
// ============================================
void StyledInput(const char* label, char* buf, size_t size, bool password = false) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 12));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme::surface);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme::surfaceHover);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::border);
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

void DrawWindowControls(ImDrawList* dl, ImVec2 ws) {
    // Minimize button
    ImVec2 minBtn(ws.x - 70, 10);
    ImVec2 minBtnSize(25, 25);
    
    ImGui::SetCursorPos(minBtn);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("_", minBtnSize)) {
        ShowWindow(g_hwnd, SW_MINIMIZE);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    
    // Close button
    ImVec2 closeBtn(ws.x - 38, 10);
    ImGui::SetCursorPos(closeBtn);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("X", minBtnSize)) {
        PostQuitMessage(0);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

// ============================================
// Screens
// ============================================
void RenderSplash() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    ImVec2 center(ws.x * 0.5f, ws.y * 0.5f);
    
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ws, 
        IM_COL32(10, 10, 18, 255), IM_COL32(10, 10, 18, 255),
        IM_COL32(5, 5, 10, 255), IM_COL32(5, 5, 10, 255));
    
    float scale = fminf(g_splashTimer / 0.5f, 1.0f);
    dl->AddCircleFilled(ImVec2(center.x, center.y - 30), 40.0f * scale, theme::gradientA, 32);
    
    if (g_splashTimer > 0.3f) {
        float alpha = fminf((g_splashTimer - 0.3f) / 0.4f, 1.0f);
        const char* title = PROJECT_NAME;
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y + 30), 
            IM_COL32(255, 255, 255, (int)(alpha * 255)), title);
    }
    
    float progress = fminf(g_splashTimer / 2.0f, 1.0f);
    ImVec2 barPos(center.x - 90, center.y + 70);
    dl->AddRectFilled(barPos, ImVec2(barPos.x + 180, barPos.y + 4), IM_COL32(40, 40, 60, 255), 2.0f);
    dl->AddRectFilled(barPos, ImVec2(barPos.x + 180 * progress, barPos.y + 4), theme::gradientA, 2.0f);
    
    if (g_splashTimer > 2.2f) {
        LoadCredentials();
        if (LoadSession()) {
            FetchUserLicenses();
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
    
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ws, 
        IM_COL32(10, 10, 18, 255), IM_COL32(10, 10, 18, 255),
        IM_COL32(5, 5, 10, 255), IM_COL32(5, 5, 10, 255));
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float w = 300.0f;
    float x = (ws.x - w) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    if (ImGui::Begin("##login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Window controls
        DrawWindowControls(dl, ws);
        
        // Logo + Project name
        dl->AddCircleFilled(ImVec2(ws.x * 0.5f, 80), 32.0f, theme::gradientA, 32);
        
        ImGui::SetCursorPos(ImVec2(0, 125));
        float tw = ImGui::CalcTextSize(PROJECT_NAME).x;
        ImGui::SetCursorPosX((ws.x - tw) * 0.5f);
        ImGui::TextColored(theme::text, PROJECT_NAME);
        
        ImGui::SetCursorPosX((ws.x - ImGui::CalcTextSize("Sign in to continue").x) * 0.5f);
        ImGui::TextColored(theme::textDim, "Sign in to continue");
        
        ImGui::Dummy(ImVec2(0, 20));
        
        // Inputs
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(theme::textSec, "Username");
        ImGui::SetCursorPosX(x);
        ImGui::SetNextItemWidth(w);
        StyledInput("##user", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 8));
        
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(theme::textSec, "Password");
        ImGui::SetCursorPosX(x);
        ImGui::SetNextItemWidth(w);
        StyledInput("##pass", g_password, sizeof(g_password), true);
        
        // Remember me
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::SetCursorPosX(x);
        ImGui::Checkbox("Remember me", &g_rememberLogin);
        
        ImGui::Dummy(ImVec2(0, 15));
        
        // Login button
        ImGui::SetCursorPosX(x);
        if (StyledButton(g_isLoading ? "  Signing in...  " : "  SIGN IN  ", ImVec2(w, 46), !g_isLoading)) {
            DoLogin();
        }
        
        // Error/success messages
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::SetCursorPosX((ws.x - ImGui::CalcTextSize(g_errorMsg.c_str()).x) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        if (!g_successMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::SetCursorPosX((ws.x - ImGui::CalcTextSize(g_successMsg.c_str()).x) * 0.5f);
            ImGui::TextColored(theme::success, "%s", g_successMsg.c_str());
        }
        
        // Register link - fixed positioning
        ImGui::SetCursorPos(ImVec2(0, ws.y - 60));
        ImGui::SetCursorPosX((ws.x - 200) * 0.5f);
        ImGui::TextColored(theme::textDim, "No account?");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, theme::accent);
        if (ImGui::SmallButton("Register")) {
            g_currentScreen = Screen::Register;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
            g_successMsg = "";
        }
        ImGui::PopStyleColor(2);
        
        // Version - separate line
        ImGui::SetCursorPos(ImVec2(0, ws.y - 28));
        ImGui::SetCursorPosX((ws.x - 60) * 0.5f);
        ImGui::TextColored(theme::textDim, "v" LAUNCHER_VERSION);
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void RenderRegister() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ws, 
        IM_COL32(10, 10, 18, 255), IM_COL32(10, 10, 18, 255),
        IM_COL32(5, 5, 10, 255), IM_COL32(5, 5, 10, 255));
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    float w = 300.0f;
    float x = (ws.x - w) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    if (ImGui::Begin("##reg", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // Window controls
        DrawWindowControls(dl, ws);
        
        // Back button
        ImGui::SetCursorPos(ImVec2(15, 15));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("<", ImVec2(30, 30))) {
            g_currentScreen = Screen::Login;
            g_fadeAlpha = 0.0f;
            g_errorMsg = "";
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // Logo + Name
        dl->AddCircleFilled(ImVec2(ws.x * 0.5f, 70), 28.0f, theme::gradientA, 32);
        
        ImGui::SetCursorPos(ImVec2(0, 110));
        float tw = ImGui::CalcTextSize(PROJECT_NAME).x;
        ImGui::SetCursorPosX((ws.x - tw) * 0.5f);
        ImGui::TextColored(theme::text, PROJECT_NAME);
        
        ImGui::SetCursorPosX((ws.x - ImGui::CalcTextSize("Create new account").x) * 0.5f);
        ImGui::TextColored(theme::textDim, "Create new account");
        
        ImGui::Dummy(ImVec2(0, 15));
        
        // Inputs
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(theme::textSec, "License Key");
        ImGui::SetCursorPosX(x);
        ImGui::SetNextItemWidth(w);
        StyledInput("##key", g_licenseKey, sizeof(g_licenseKey));
        
        ImGui::Dummy(ImVec2(0, 6));
        
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(theme::textSec, "Username");
        ImGui::SetCursorPosX(x);
        ImGui::SetNextItemWidth(w);
        StyledInput("##ruser", g_username, sizeof(g_username));
        
        ImGui::Dummy(ImVec2(0, 6));
        
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(theme::textSec, "Password");
        ImGui::SetCursorPosX(x);
        ImGui::SetNextItemWidth(w);
        StyledInput("##rpass", g_password, sizeof(g_password), true);
        
        ImGui::Dummy(ImVec2(0, 15));
        
        // Register button
        ImGui::SetCursorPosX(x);
        if (StyledButton(g_isLoading ? "  Creating...  " : "  CREATE ACCOUNT  ", ImVec2(w, 46), !g_isLoading)) {
            DoRegister();
        }
        
        // Error message
        if (!g_errorMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::SetCursorPosX((ws.x - ImGui::CalcTextSize(g_errorMsg.c_str()).x) * 0.5f);
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void RenderMain() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    
    g_fadeAlpha = fminf(g_fadeAlpha + ImGui::GetIO().DeltaTime * 3.0f, 1.0f);
    
    // Background
    dl->AddRectFilled(ImVec2(0, 0), ws, IM_COL32(10, 10, 15, 255));
    
    // Sidebar
    float sidebarW = 180.0f;
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(sidebarW, ws.y), IM_COL32(15, 15, 22, 255));
    dl->AddLine(ImVec2(sidebarW, 0), ImVec2(sidebarW, ws.y), IM_COL32(40, 40, 60, 255));
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fadeAlpha);
        
        // ===== SIDEBAR =====
        // Logo
        ImGui::SetCursorPos(ImVec2(20, 20));
        dl->AddCircleFilled(ImVec2(40, 40), 18, theme::gradientA, 24);
        ImGui::SetCursorPos(ImVec2(65, 22));
        ImGui::TextColored(theme::text, "Single");
        ImGui::SetCursorPos(ImVec2(65, 38));
        ImGui::TextColored(theme::textSec, "Project");
        
        // User
        ImGui::SetCursorPos(ImVec2(15, 70));
        dl->AddRectFilled(ImVec2(10, 65), ImVec2(sidebarW - 10, 105), IM_COL32(25, 25, 35, 255), 8.0f);
        ImGui::SetCursorPos(ImVec2(20, 73));
        ImGui::TextColored(theme::textSec, "Welcome,");
        ImGui::SetCursorPos(ImVec2(20, 88));
        ImGui::TextColored(theme::text, "%s", g_displayName.c_str());
        
        // Games list
        ImGui::SetCursorPos(ImVec2(15, 120));
        ImGui::TextColored(theme::textDim, "GAMES");
        
        float gameY = 140;
        for (int i = 0; i < (int)g_games.size(); i++) {
            const auto& game = g_games[i];
            bool selected = (i == g_selectedGame);
            
            ImVec2 btnPos(10, gameY);
            ImVec2 btnSize(sidebarW - 20, 50);
            
            ImU32 bgColor = selected ? IM_COL32(140, 90, 245, 40) : IM_COL32(0, 0, 0, 0);
            if (!selected) {
                ImVec2 mousePos = ImGui::GetMousePos();
                if (mousePos.x >= btnPos.x && mousePos.x <= btnPos.x + btnSize.x &&
                    mousePos.y >= btnPos.y && mousePos.y <= btnPos.y + btnSize.y) {
                    bgColor = IM_COL32(40, 40, 55, 255);
                }
            }
            
            dl->AddRectFilled(btnPos, ImVec2(btnPos.x + btnSize.x, btnPos.y + btnSize.y), bgColor, 8.0f);
            
            if (selected) {
                dl->AddRectFilled(ImVec2(0, btnPos.y), ImVec2(3, btnPos.y + btnSize.y), theme::gradientA);
            }
            
            // Icon
            dl->AddCircleFilled(ImVec2(btnPos.x + 22, btnPos.y + 25), 14, 
                game.available ? theme::gradientA : IM_COL32(60, 60, 80, 255), 20);
            ImVec2 iconSize = ImGui::CalcTextSize(game.icon.c_str());
            dl->AddText(ImVec2(btnPos.x + 22 - iconSize.x * 0.5f, btnPos.y + 25 - iconSize.y * 0.5f),
                IM_COL32(255, 255, 255, game.available ? 255 : 150), game.icon.c_str());
            
            // Name
            dl->AddText(ImVec2(btnPos.x + 45, btnPos.y + 10), 
                IM_COL32(255, 255, 255, game.available ? 255 : 120), game.name.c_str());
            
            // Status
            const char* status = game.hasLicense 
                ? (game.daysRemaining < 0 ? "Lifetime" : "Active")
                : (game.available ? "No license" : "Coming soon");
            ImU32 statusColor = game.hasLicense 
                ? IM_COL32(50, 220, 120, 255) 
                : (game.available ? IM_COL32(255, 180, 50, 255) : IM_COL32(100, 100, 120, 255));
            dl->AddText(ImVec2(btnPos.x + 45, btnPos.y + 28), statusColor, status);
            
            // Click
            ImGui::SetCursorPos(btnPos);
            ImGui::InvisibleButton(("game" + std::to_string(i)).c_str(), btnSize);
            if (ImGui::IsItemClicked()) {
                g_selectedGame = i;
                g_errorMsg = "";
                g_successMsg = "";
            }
            
            gameY += 55;
        }
        
        // Logout
        ImGui::SetCursorPos(ImVec2(10, ws.y - 50));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("Logout", ImVec2(sidebarW - 20, 36))) {
            DoLogout();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            return;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // ===== CONTENT AREA =====
        float contentX = sidebarW + 30;
        float contentW = ws.x - sidebarW - 60;
        
        GameInfo& game = g_games[g_selectedGame];
        
        // Header with close/minimize
        ImGui::SetCursorPos(ImVec2(ws.x - 70, 10));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("_##min", ImVec2(25, 25))) {
            ShowWindow(g_hwnd, SW_MINIMIZE);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        ImGui::SetCursorPos(ImVec2(ws.x - 38, 10));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("X##close", ImVec2(25, 25))) {
            PostQuitMessage(0);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // Game title
        ImGui::SetCursorPos(ImVec2(contentX, 20));
        ImGui::TextColored(theme::text, "%s", game.name.c_str());
        ImGui::SetCursorPos(ImVec2(contentX, 40));
        ImGui::TextColored(theme::textDim, "v%s", game.version.c_str());
        
        // License card
        ImVec2 licCardPos(contentX, 70);
        ImVec2 licCardSize(contentW, 65);
        dl->AddRectFilled(licCardPos, ImVec2(licCardPos.x + licCardSize.x, licCardPos.y + licCardSize.y),
            IM_COL32(20, 20, 30, 255), 12.0f);
        
        ImGui::SetCursorPos(ImVec2(licCardPos.x + 16, licCardPos.y + 10));
        ImGui::TextColored(theme::textSec, "License Status");
        
        ImGui::SetCursorPos(ImVec2(licCardPos.x + 16, licCardPos.y + 32));
        if (game.hasLicense) {
            ImGui::TextColored(theme::success, "ACTIVE");
            ImGui::SameLine();
            if (game.daysRemaining < 0) {
                ImGui::TextColored(theme::textSec, "- Lifetime");
            } else {
                ImGui::TextColored(theme::textSec, "- %d days", game.daysRemaining);
            }
        } else {
            ImGui::TextColored(theme::error, "NO LICENSE");
        }
        
        float nextY = licCardPos.y + licCardSize.y + 12;
        
        // Activate license (if no license)
        if (!game.hasLicense && game.available) {
            ImVec2 actPos(contentX, nextY);
            ImVec2 actSize(contentW, 70);
            dl->AddRectFilled(actPos, ImVec2(actPos.x + actSize.x, actPos.y + actSize.y),
                IM_COL32(25, 20, 35, 255), 12.0f);
            
            ImGui::SetCursorPos(ImVec2(actPos.x + 16, actPos.y + 10));
            ImGui::TextColored(theme::accent, "Activate License");
            
            ImGui::SetCursorPos(ImVec2(actPos.x + 16, actPos.y + 32));
            ImGui::SetNextItemWidth(actSize.x - 130);
            StyledInput("##actkey", g_activateKey, sizeof(g_activateKey));
            
            ImGui::SameLine();
            if (StyledButton("Activate", ImVec2(90, 34), !g_isLoading)) {
                ActivateLicense(game.id);
            }
            
            nextY = actPos.y + actSize.y + 12;
        }
        
        // Description card (instead of Features)
        ImVec2 descPos(contentX, nextY);
        ImVec2 descSize(contentW, 55);
        dl->AddRectFilled(descPos, ImVec2(descPos.x + descSize.x, descPos.y + descSize.y),
            IM_COL32(20, 20, 30, 255), 12.0f);
        
        ImGui::SetCursorPos(ImVec2(descPos.x + 16, descPos.y + 10));
        ImGui::TextColored(theme::textSec, "Included:");
        
        ImGui::SetCursorPos(ImVec2(descPos.x + 16, descPos.y + 30));
        if (game.available) {
            ImGui::TextColored(theme::success, "%s", game.description.c_str());
        } else {
            ImGui::TextColored(theme::textDim, "Coming soon...");
        }
        
        // Launch button
        nextY = descPos.y + descSize.y + 15;
        ImGui::SetCursorPos(ImVec2(contentX, nextY));
        
        if (g_isDownloading) {
            ImVec2 barPos(contentX, nextY + 5);
            dl->AddRectFilled(barPos, ImVec2(barPos.x + contentW, barPos.y + 45),
                IM_COL32(30, 30, 45, 255), 10.0f);
            dl->AddRectFilled(barPos, ImVec2(barPos.x + contentW * g_downloadProgress, barPos.y + 45),
                theme::gradientA, 10.0f);
            
            char progText[32];
            sprintf_s(progText, "%.0f%%", g_downloadProgress * 100.0f);
            ImVec2 ptSize = ImGui::CalcTextSize(progText);
            dl->AddText(ImVec2(barPos.x + (contentW - ptSize.x) * 0.5f, barPos.y + 12),
                IM_COL32(255, 255, 255, 255), progText);
        } else {
            bool canLaunch = game.hasLicense && game.available && !g_cheatRunning;
            std::string btnLabel = g_cheatRunning ? "  RUNNING  " : "  LAUNCH  ";
            if (StyledButton(btnLabel.c_str(), ImVec2(contentW, 50), canLaunch)) {
                LaunchGame(g_selectedGame);
            }
        }
        
        // Messages
        nextY += 65;
        if (!g_errorMsg.empty()) {
            ImGui::SetCursorPos(ImVec2(contentX, nextY));
            ImGui::TextColored(theme::error, "%s", g_errorMsg.c_str());
        }
        if (!g_successMsg.empty()) {
            ImGui::SetCursorPos(ImVec2(contentX, nextY));
            ImGui::TextColored(theme::success, "%s", g_successMsg.c_str());
        }
        
        // Footer
        ImGui::SetCursorPos(ImVec2(contentX, ws.y - 25));
        ImGui::TextColored(theme::textDim, PROJECT_NAME " v" LAUNCHER_VERSION);
        
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ============================================
// Main
// ============================================
void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0;
    style.FrameRounding = 8.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(12, 10);
    style.ItemSpacing = ImVec2(10, 8);
    
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = theme::bg;
    c[ImGuiCol_FrameBg] = theme::surface;
    c[ImGuiCol_FrameBgHovered] = theme::surfaceHover;
    c[ImGuiCol_Button] = theme::accent;
    c[ImGuiCol_ButtonHovered] = theme::accentHover;
    c[ImGuiCol_Text] = theme::text;
    c[ImGuiCol_CheckMark] = theme::accent;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // ============================================
    // PROTECTION CHECKS
    // ============================================
    int protResult = Protection::RunProtectionChecks();
    
    if (protResult == 1) {
        // Debugger detected - silent exit
        Protection::SecureExit(0);
    }
    
    if (protResult == 2 && !g_bypassVM) {
        // VM detected - show warning (can be bypassed for dev)
        MessageBoxA(NULL, "This software cannot run in virtual machines.", 
                   "Security", MB_ICONWARNING | MB_OK);
        Protection::SecureExit(0);
    }
    
    if (protResult == 3 || protResult == 4) {
        // Integrity/hooks failed - silent exit
        Protection::SecureExit(0);
    }
    
    g_protectionPassed = true;
    
    // ============================================
    // Normal startup
    // ============================================
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"SingleProject", nullptr };
    RegisterClassExW(&wc);
    
    g_hwnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"Single-Project",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,
        WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, wc.hInstance, nullptr);
    
    DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
    DwmSetWindowAttribute(g_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
    
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
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f);
    
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    SetupStyle();
    g_hwid = GetHWID();
    
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    static int protCheckCounter = 0;
    
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        // Periodic protection check (every ~5 seconds at 60fps)
        if (++protCheckCounter > 300) {
            protCheckCounter = 0;
            if (Protection::IsDebuggerAttached() || Protection::HasDebuggerWindows()) {
                Protection::SecureExit(0);
            }
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        
        g_animTimer += dt;
        if (g_currentScreen == Screen::Splash) g_splashTimer += dt;
        
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
// DirectX
// ============================================
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = {60, 1};
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;
    
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, 
        levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, 
        &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
    
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
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    
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
            if (pt.y < 50 && pt.x > 180 && pt.x < WINDOW_WIDTH - 80) return HTCAPTION;
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
