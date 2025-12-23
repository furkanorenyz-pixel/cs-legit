/**
 * CS-Legit Launcher
 * External Cheat for CS2
 */

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif

#include "../include/api.hpp"

namespace fs = std::filesystem;
using namespace launcher;

// ============================================
// Config
// ============================================

const std::string SERVER_URL = "http://single-project.duckdns.org";
const std::string CHEAT_DIR = "files";
const std::string CONFIG_FILE = "session.dat";
const std::string VERSION = "2.0.0";

// ============================================
// Console Helpers
// ============================================

void ClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void SetColor(int color) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
}

void Print(const std::string& msg, int color = 7) {
    SetColor(color);
    std::cout << msg;
    SetColor(7);
}

void PrintLine(const std::string& msg, int color = 7) {
    Print(msg + "\n", color);
}

void PrintLogo() {
    SetColor(13); // Purple
    std::cout << R"(
   ██████╗███████╗      ██╗     ███████╗ ██████╗ ██╗████████╗
  ██╔════╝██╔════╝      ██║     ██╔════╝██╔════╝ ██║╚══██╔══╝
  ██║     ███████╗█████╗██║     █████╗  ██║  ███╗██║   ██║   
  ██║     ╚════██║╚════╝██║     ██╔══╝  ██║   ██║██║   ██║   
  ╚██████╗███████║      ███████╗███████╗╚██████╔╝██║   ██║   
   ╚═════╝╚══════╝      ╚══════╝╚══════╝ ╚═════╝ ╚═╝   ╚═╝   
    )" << std::endl;
    SetColor(8);
    std::cout << "  Launcher v" << VERSION << " | External ESP for CS2\n";
    SetColor(7);
    std::cout << "  ───────────────────────────────────────────────────\n\n";
}

void PrintError(const std::string& msg) {
    PrintLine("  [!] " + msg, 12);
}

void PrintSuccess(const std::string& msg) {
    PrintLine("  [+] " + msg, 10);
}

void PrintInfo(const std::string& msg) {
    PrintLine("  [*] " + msg, 14);
}

void WaitEnter() {
    std::cout << "\n  Press Enter to continue...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}

// ============================================
// Session Management
// ============================================

struct Session {
    std::string token;
    std::string username;
};

void SaveSession(const Session& session) {
    std::ofstream file(CONFIG_FILE, std::ios::binary);
    if (file.is_open()) {
        size_t len = session.token.length();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(session.token.c_str(), len);
        len = session.username.length();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(session.username.c_str(), len);
        file.close();
    }
}

Session LoadSession() {
    Session session;
    std::ifstream file(CONFIG_FILE, std::ios::binary);
    if (file.is_open()) {
        size_t len;
        if (file.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            session.token.resize(len);
            file.read(&session.token[0], len);
        }
        if (file.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            session.username.resize(len);
            file.read(&session.username[0], len);
        }
        file.close();
    }
    return session;
}

void ClearSession() {
    fs::remove(CONFIG_FILE);
}

// ============================================
// Process Helpers
// ============================================

#ifdef _WIN32
bool IsProcessRunning(const std::string& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    
    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, processName.c_str()) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return false;
}

void OpenUrl(const std::string& url) {
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
#endif

// ============================================
// Login Screen
// ============================================

bool DoLogin(ApiClient& api, Session& session) {
    std::string username, password;
    
    Print("  Username: ", 7);
    std::cin >> username;
    Print("  Password: ", 7);
    
    // Hide password input
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
#endif
    
    std::cin >> password;
    
#ifdef _WIN32
    SetConsoleMode(hStdin, mode);
#endif
    std::cout << std::endl;
    
    PrintInfo("Connecting...");
    
    auto result = api.Login(username, password);
    
    if (result.success) {
        session.token = result.token;
        session.username = result.user.username;
        SaveSession(session);
        PrintSuccess("Welcome back, " + session.username + "!");
        return true;
    } else {
        PrintError(result.error);
        return false;
    }
}

// ============================================
// Register Screen
// ============================================

bool DoRegister(ApiClient& api, Session& session) {
    std::string username, password, license;
    
    std::cout << "\n";
    PrintLine("  ═══════════════════════════════════════════════", 13);
    PrintLine("                  REGISTRATION", 13);
    PrintLine("  ═══════════════════════════════════════════════", 13);
    std::cout << "\n";
    
    Print("  License Key: ", 14);
    std::cin >> license;
    
    Print("  Username: ", 7);
    std::cin >> username;
    
    Print("  Password: ", 7);
    std::cin >> password;
    
    std::cout << std::endl;
    PrintInfo("Registering...");
    
    auto result = api.Register(username, password, license);
    
    if (result.success) {
        session.token = result.token;
        session.username = result.user.username;
        SaveSession(session);
        PrintSuccess("Registration successful!");
        PrintSuccess("Welcome, " + session.username + "!");
        return true;
    } else {
        PrintError(result.error);
        return false;
    }
}

// ============================================
// Main Menu (After Login)
// ============================================

void ShowInstructions() {
    ClearScreen();
    PrintLogo();
    
    PrintLine("  ═══════════════════════════════════════════════", 14);
    PrintLine("              HOW TO USE EXTERNAL ESP", 14);
    PrintLine("  ═══════════════════════════════════════════════", 14);
    std::cout << "\n";
    
    PrintLine("  1. Launch CS2 and wait for main menu", 7);
    PrintLine("  2. Press [LAUNCH] in this launcher", 7);
    PrintLine("  3. External overlay will appear on top of game", 7);
    std::cout << "\n";
    
    PrintLine("  HOTKEYS:", 10);
    PrintLine("  ─────────────────────────────────────", 8);
    PrintLine("  INSERT     - Open/Close menu", 7);
    PrintLine("  HOME       - Toggle ESP", 7);
    PrintLine("  END        - Exit cheat", 7);
    std::cout << "\n";
    
    PrintLine("  FEATURES:", 10);
    PrintLine("  ─────────────────────────────────────", 8);
    PrintLine("  • Box ESP (2D/3D/Corner)", 7);
    PrintLine("  • Health bar", 7);
    PrintLine("  • Player names", 7);
    PrintLine("  • Distance", 7);
    PrintLine("  • Skeleton ESP", 7);
    PrintLine("  • Head dot", 7);
    PrintLine("  • Snaplines", 7);
    std::cout << "\n";
    
    PrintLine("  NOTES:", 14);
    PrintLine("  ─────────────────────────────────────", 8);
    PrintLine("  • Run CS2 in WINDOWED or BORDERLESS mode", 7);
    PrintLine("  • Overlay is external - undetected", 7);
    PrintLine("  • Offsets update automatically", 7);
    
    WaitEnter();
}

void LaunchExternal(ApiClient& api) {
    ClearScreen();
    PrintLogo();
    
    // Check CS2 is running
#ifdef _WIN32
    if (!IsProcessRunning("cs2.exe")) {
        PrintError("CS2 is not running!");
        PrintInfo("Please start CS2 first, then press Launch.");
        WaitEnter();
        return;
    }
#endif
    
    PrintSuccess("CS2 detected!");
    
    // Create directory
    fs::create_directories(CHEAT_DIR);
    
    std::string exePath = CHEAT_DIR + "/externa.exe";
    std::string versionPath = CHEAT_DIR + "/version.json";
    
    // Check for updates
    PrintInfo("Checking for updates...");
    
    std::string localVersion = "";
    if (fs::exists(versionPath)) {
        std::ifstream vf(versionPath);
        std::string content((std::istreambuf_iterator<char>(vf)),
                           std::istreambuf_iterator<char>());
        // Simple parse for version
        auto pos = content.find("\"version\"");
        if (pos != std::string::npos) {
            auto start = content.find("\"", pos + 10) + 1;
            auto end = content.find("\"", start);
            localVersion = content.substr(start, end - start);
        }
    }
    
    bool needsDownload = !fs::exists(exePath);
    
    // Check server version
    auto gameInfo = api.GetGame("cs2");
    if (gameInfo.has_value()) {
        if (localVersion != gameInfo->latest_version) {
            needsDownload = true;
            PrintInfo("New version available: " + gameInfo->latest_version);
        }
    }
    
    if (needsDownload) {
        PrintInfo("Downloading latest version...");
        
        bool success = api.DownloadFile("cs2", "externa", exePath, 
            [](size_t current, size_t total) {
                int percent = (total > 0) ? (int)(current * 100 / total) : 0;
                std::cout << "\r  Progress: " << percent << "%   " << std::flush;
            });
        
        std::cout << std::endl;
        
        if (!success) {
            PrintError("Download failed! Check your internet connection.");
            WaitEnter();
            return;
        }
        
        // Save version info
        if (gameInfo.has_value()) {
            std::ofstream vf(versionPath);
            vf << "{\"version\":\"" << gameInfo->latest_version << "\"}";
        }
        
        PrintSuccess("Download complete!");
    } else {
        PrintSuccess("Cheat is up to date (v" + localVersion + ")");
    }
    
    // Download offsets
    PrintInfo("Updating offsets...");
    std::string offsets = api.GetOffsets("cs2");
    if (!offsets.empty()) {
        std::ofstream of(CHEAT_DIR + "/offsets.json");
        of << offsets;
        PrintSuccess("Offsets updated!");
    }
    
    // Launch
    PrintInfo("Launching externa.exe...");
    
#ifdef _WIN32
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (CreateProcessA(exePath.c_str(), NULL, NULL, NULL, FALSE, 
                       0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        PrintSuccess("Launched successfully!");
        PrintLine("\n  The overlay should now appear over CS2.", 10);
        PrintLine("  Press INSERT to open the menu.", 14);
    } else {
        PrintError("Failed to launch! Error: " + std::to_string(GetLastError()));
    }
#else
    PrintError("Windows only feature.");
#endif
    
    WaitEnter();
}

void MainMenu(ApiClient& api, Session& session) {
    while (true) {
        ClearScreen();
        PrintLogo();
        
        // User info
        Print("  Logged in as: ", 8);
        PrintLine(session.username, 10);
        std::cout << "\n";
        
        // Menu
        PrintLine("  ╔═══════════════════════════════════════════╗", 13);
        PrintLine("  ║                                           ║", 13);
        Print("  ║   ", 13);
        Print("[1]", 14);
        Print("  LAUNCH EXTERNAL              ", 7);
        PrintLine("║", 13);
        PrintLine("  ║                                           ║", 13);
        Print("  ║   ", 13);
        Print("[2]", 8);
        Print("  How to Use                       ", 8);
        PrintLine("║", 13);
        Print("  ║   ", 13);
        Print("[3]", 8);
        Print("  Check for Updates                ", 8);
        PrintLine("║", 13);
        Print("  ║   ", 13);
        Print("[4]", 8);
        Print("  Logout                           ", 8);
        PrintLine("║", 13);
        Print("  ║   ", 13);
        Print("[0]", 12);
        Print("  Exit                             ", 8);
        PrintLine("║", 13);
        PrintLine("  ║                                           ║", 13);
        PrintLine("  ╚═══════════════════════════════════════════╝", 13);
        
        std::cout << "\n  Choice: ";
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                LaunchExternal(api);
                break;
            case 2:
                ShowInstructions();
                break;
            case 3:
                PrintInfo("Checking for updates...");
                {
                    auto game = api.GetGame("cs2");
                    if (game.has_value()) {
                        PrintSuccess("Latest version: " + game->latest_version);
                    }
                }
                WaitEnter();
                break;
            case 4:
                ClearSession();
                PrintInfo("Logged out.");
                return;
            case 0:
                exit(0);
        }
    }
}

// ============================================
// Entry Point
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleTitleA("CS-Legit Launcher");
    
    // Set console size
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT rect = {0, 0, 60, 35};
    SetConsoleWindowInfo(hOut, TRUE, &rect);
#endif
    
    ApiClient api;
    api.SetServerUrl(SERVER_URL);
    
    Session session;
    bool loggedIn = false;
    
    // Try to restore session
    session = LoadSession();
    if (!session.token.empty()) {
        api.SetToken(session.token);
        
        ClearScreen();
        PrintLogo();
        PrintInfo("Restoring session...");
        
        if (api.VerifyToken()) {
            PrintSuccess("Session restored!");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            loggedIn = true;
        } else {
            PrintInfo("Session expired. Please login again.");
            ClearSession();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    // Login/Register loop
    while (!loggedIn) {
        ClearScreen();
        PrintLogo();
        
        PrintLine("  [1] Login", 7);
        PrintLine("  [2] Register (need license key)", 14);
        PrintLine("  [0] Exit", 8);
        
        std::cout << "\n  Choice: ";
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                std::cout << "\n";
                if (DoLogin(api, session)) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    loggedIn = true;
                } else {
                    WaitEnter();
                }
                break;
            case 2:
                if (DoRegister(api, session)) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    loggedIn = true;
                } else {
                    WaitEnter();
                }
                break;
            case 0:
                return 0;
        }
    }
    
    // Main menu
    MainMenu(api, session);
    
    return 0;
}
